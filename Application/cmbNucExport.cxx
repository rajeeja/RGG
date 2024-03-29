#ifndef cmbNucExport_cxx
#define cmbNucExport_cxx //incase this is included for template instant

#include <remus/client/Client.h>

#include <remus/worker/Worker.h>

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>
#include <remus/common/MeshTypes.h>
#include <remus/common/ExecuteProcess.h>
//#include <remus/client/JobResult.h>
#include <remus/proto/JobResult.h>

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QMutexLocker>
#include <QEventLoop>
#include <QThread>
#include <QThreadPool>

#include <iostream>
#include <sstream>
#include <fstream>

#include <stdlib.h>

#include "cmbNucExport.h"

namespace
{
class Thread : public QThread
{
public:
  static void msleep(int ms)
  {
    QThread::msleep(ms);
  }
};
}

class cmbNucExporterClient;

class cmbNucExporterWorker: public remus::worker::Worker
{
public:
  cmbNucExporterWorker( std::string label, remus::common::MeshIOType miotype,
                        RunnableConnection & rconn,
                        remus::worker::ServerConnection const& sconn,
                        cmbNucExport * exporter,
                        std::vector<std::string> extra_args = std::vector<std::string>());
  ~cmbNucExporterWorker();
  RunnableConnection & connection;
  void process();
  bool pollStatus( remus::common::ExecuteProcess* process,
                   const remus::worker::Job& job );
  std::string label;
  cmbNucExport * exporter;
  std::vector<std::string> ExtraArgs;
  bool keepGoing;
  QMutex syncronizer;
};

class cmbNucExportWorkerRunner: public QRunnable
{
public:
  cmbNucExportWorkerRunner( std::string l, remus::common::MeshIOType miotype,
                            //remus::worker::ServerConnection const& conn,
                            cmbNucExport * e,
                            std::vector<std::string> extra_args = std::vector<std::string>())
  :IOType(miotype), ExtraArgs(extra_args), exporter(e), label(l)
  {
  }
  ~cmbNucExportWorkerRunner()
  {}

  virtual void run()
  {
    remus::worker::ServerConnection serverConnection = this->exporter->make_ServerConnection();
    cmbNucExporterWorker worker(this->label, this->IOType, this->connection,
                                serverConnection, this->exporter, this->ExtraArgs);
    worker.process();
  }

  remus::common::MeshIOType IOType;
  RunnableConnection connection;
  std::vector<std::string> ExtraArgs;
  cmbNucExport * exporter;
  std::string label;
};

class ExporterWorkerFactory: public remus::server::WorkerFactoryBase
{
public:
  ExporterWorkerFactory( cmbNucExport * e, int maxThreadCount )
  :exporter(e)
  {
    this->setMaxWorkerCount(maxThreadCount);
    types[0] = remus::proto::make_JobRequirements(remus::common::MeshIOType("ASSYGEN_IN", "CUBIT_IN"), "Assygen", "");
    types[1] = remus::proto::make_JobRequirements(remus::common::MeshIOType("CUBIT_IN", "COREGEN_IN"), "Cubit", "");
    types[2] = remus::proto::make_JobRequirements(remus::common::MeshIOType("COREGEN_IN", "COREGEN_OUT"), "Coregen", "");
    threadPool.setMaxThreadCount( maxThreadCount );
    threadPool.setExpiryTimeout( -1 );
  }

  virtual remus::proto::JobRequirementsSet workerRequirements(remus::common::MeshIOType type) const
  {
    remus::proto::JobRequirementsSet result;
    if(types[0].meshTypes() == type) result.insert(types[0]);
    else if(types[1].meshTypes() == type) result.insert(types[1]);
    else if(types[2].meshTypes() == type) result.insert(types[2]);
    return result;
  }

  virtual remus::common::MeshIOTypeSet supportedIOTypes() const
  {
    remus::common::MeshIOTypeSet result;
    result.insert(types[0].meshTypes());
    result.insert(types[1].meshTypes());
    result.insert(types[2].meshTypes());
    return result;
  }

  virtual bool haveSupport(const remus::proto::JobRequirements& reqs) const
  {
    return reqs == types[0] || reqs == types[1] || reqs == types[2];
  }

  virtual bool createWorker(const remus::proto::JobRequirements& reqs,
                            remus::server::WorkerFactory::FactoryDeletionBehavior /*lifespan*/)
  {
    if(this->currentWorkerCount() >= this->maxWorkerCount()) return false;
    //remus::worker::ServerConnection connection = remus::worker::ServerConnection();
    cmbNucExportWorkerRunner * runner;
    if(reqs == types[0])
    {
      //create and connect assygen worker
      runner = new cmbNucExportWorkerRunner( "Assygen", reqs.meshTypes(), /*connection,*/ exporter);
    }
    else if(reqs == types[1])
    {
      //create and connect cubit worker
      std::vector<std::string> args;
      args.push_back("-nographics");
      args.push_back("-batch");
      runner = new cmbNucExportWorkerRunner( "Cubit", reqs.meshTypes(), /*connection,*/ exporter, args);
    }
    else if(reqs == types[2])
    {
      //create and connect coregen worker
      runner = new cmbNucExportWorkerRunner( "Coregen", reqs.meshTypes(), /*connection,*/ exporter);
    }
    else
    {
      return false;
    }
    QObject::connect( &(runner->connection), SIGNAL(currentMessage(QString)), exporter, SIGNAL(statusMessage(QString)) );
    qDebug() << threadPool.maxThreadCount();
    threadPool.start(runner);
    return true;
  }

  virtual void updateWorkerCount()
  {
  }

  virtual unsigned int currentWorkerCount() const
  {
    unsigned int tpc = threadPool.activeThreadCount();
    return tpc;
  }

  void setMaxThreadCount(int maxThreadCount)
  {
    threadPool.setMaxThreadCount( maxThreadCount );
    this->setMaxWorkerCount(maxThreadCount);
  }

protected:
  remus::proto::JobRequirements types[3];
  QThreadPool threadPool;
  cmbNucExport * exporter;
};

struct ExporterInput
{
  std::string ExeDir;
  std::string Function;
  std::string LibPath;
  std::string FileArg;
  std::string OutputFile;
  operator std::string(void) const
  {
    std::stringstream ss;
    ss << ExeDir << ';' << FileArg << ";" <<Function << ";" << LibPath << ';' << OutputFile;
    return ss.str();
  }
  ExporterInput(QString exeDir, QString fun, QString file, QString of)
  :ExeDir(exeDir.trimmed().toStdString()), Function(fun.trimmed().toStdString()),
   FileArg(file.trimmed().toStdString()), OutputFile(of.toStdString())
  {  }
  ExporterInput(const remus::worker::Job& job)
  {
    //qDebug() << remus::worker::to_string(job).c_str();
    std::stringstream ss(job.details("data"));
    getline(ss, ExeDir,   ';');
    getline(ss, FileArg,  ';');
    getline(ss, Function, ';');
    getline(ss, LibPath,  ';');
    getline(ss, OutputFile,  ';');
  }
};

struct ExporterOutput
{
  bool Valid;
  std::string Result;
  ExporterOutput():Valid(false){}
  ExporterOutput(const remus::proto::JobResult& job, bool valid=true)
  : Valid(valid), Result(job.data())
  {}
  operator std::string(void) const
  { return Result; }
};

class cmbNucExporterClient
{
public:
  cmbNucExporterClient(remus::client::ServerConnection conn);
  ~cmbNucExporterClient()
  {delete Client;}
  bool getOutput(std::string label, std::string itype, std::string otype,
                 ExporterInput const& in, remus::proto::Job ** job);
  remus::proto::JobStatus jobStatus(remus::proto::Job * job)
  {
    return Client->jobStatus(*job);
  }
private:
  remus::client::ServerConnection Connection;
  remus::Client * Client;
  std::string Label;
  std::string input_type;
  std::string output_type;
};

struct JobHolder
{
  JobHolder(QString exeDir, QString fun, QString file, QString of)
  : running(false), done(false), in(exeDir, fun, file, of)
  {
    job = NULL;
  }
  ~JobHolder()
  {
    delete job;
  }
  bool running, done;
  std::vector<JobHolder*> dependencies;
  std::string label, itype, otype;
  remus::proto::Job * job;
  ExporterInput in;
};

cmbNucExporterWorker
::cmbNucExporterWorker( std::string l, remus::common::MeshIOType miotype,
                        RunnableConnection & rconn,
                        remus::worker::ServerConnection const& conn,
                        cmbNucExport * e,
                        std::vector<std::string> extra_args )
:remus::worker::Worker( remus::proto::make_JobRequirements(miotype, l, ""), conn),
 connection(rconn), exporter(e), ExtraArgs(extra_args),
 keepGoing(true)
{
  this->label = l;
  if(exporter!=NULL) exporter->registerWorker(this);
}

void
cmbNucExporterWorker
::process()
{
  syncronizer.lock();
  remus::worker::Job job = this->getJob();
  if(!job.valid())
  {
    switch(job.validityReason())
    {
      case remus::worker::Job::INVALID:
        connection.sendErrorMessage("JOB NOT VALID");
        return;
      case remus::worker::Job::TERMINATE_WORKER:
        return;
      case remus::worker::Job::VALID_JOB:
        ((void)0); //have valid job, move on.
    }
  }
  qDebug() << "Recieved valid job for: " << this->label.c_str();

  ExporterInput input(job);

  //remove old output file
  QFile::remove(input.OutputFile.c_str());

  //Run in execute directory
  QString current = QDir::currentPath();
  QDir::setCurrent( input.ExeDir.c_str() );

  //make a cleaned up path with no relative
  std::vector<std::string> args;
  for( std::vector<std::string>::const_iterator i = ExtraArgs.begin();
      i < ExtraArgs.end(); ++i )
  {
    args.push_back(*i);
  }
  args.push_back(input.FileArg);

#ifdef __APPLE__
  char* oldEnv = getenv("DYLD_LIBRARY_PATH");
  std::string env(input.LibPath);
  setenv("DYLD_LIBRARY_PATH", env.c_str(), 1);
#elif  __linux
  char* oldEnv = getenv("LD_LIBRARY_PATH");
  std::string env(input.LibPath);
  setenv("LD_LIBRARY_PATH", env.c_str(), 1);
#endif

  qDebug() << "starting exe: " << this->label.c_str();
  remus::common::ExecuteProcess* ep = new remus::common::ExecuteProcess( input.Function, args);

  //actually launch the new process
  ep->execute(remus::common::ExecuteProcess::Attached);

  //Wait for finish
  qDebug() << "waiting: " << this->label.c_str();
  if(pollStatus(ep, job) && QFileInfo(input.OutputFile.c_str()).exists())
  {
    qDebug() << "Done waiting.  Is ok: " << this->label.c_str();
    remus::proto::JobResult results = remus::proto::make_JobResult(job.id(),"DUMMY FOR NOW;");
    this->returnResult(results);
  }
  else
  {
    qDebug() << "Done waiting.  Job Failed: " << this->label.c_str();
    remus::proto::JobStatus status(job.id(),remus::FAILED);
    updateStatus(status);
  }
  QDir::setCurrent( current );
  delete ep;

#ifdef __APPLE__
  if(oldEnv != NULL)
  {
    setenv("DYLD_LIBRARY_PATH", oldEnv, 1);
  }
#elif  __linux
  if(oldEnv != NULL)
  {
    setenv("LD_LIBRARY_PATH", oldEnv, 1);
  }
#endif
  qDebug() << "finish: " << this->label.c_str();
  syncronizer.unlock();
  if(exporter!=NULL) exporter->workerDone(this);
}

cmbNucExporterWorker
::~cmbNucExporterWorker()
{
}

bool cmbNucExporterWorker
::pollStatus( remus::common::ExecuteProcess* ep,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::proto::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(ep->isAlive()&& validExection && this->keepGoing)
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = ep->poll(2);
    if(data.type == ProcessPipe::STDOUT || data.type == ProcessPipe::STDERR)
    {
      //we have something on the output pipe
      remus::proto::JobProgress progress(data.text);
      status.updateProgress(progress);
      connection.sendCurrentMessage( QString(data.text.c_str()) );
    }
  }
  if(ep->isAlive())
  {
    qDebug() << "Killing process";
    ep->kill();
  }

  //verify we exited normally, not segfault or numeric exception
  validExection &= ep->exitedNormally();

  if(!validExection)
  {
    return false;
  }
  return true;
}

cmbNucExporterClient::cmbNucExporterClient(remus::client::ServerConnection conn)
{
  Connection = conn;
  Client = new remus::Client(Connection);
}

bool
cmbNucExporterClient::getOutput(std::string label, std::string it, std::string ot,
                                ExporterInput const& in, remus::proto::Job ** job)
{
  ExporterOutput eo;
  remus::common::MeshIOType mesh_types(it, ot);
  remus::proto::JobRequirements reqs = remus::proto::make_JobRequirements(mesh_types, label,"");
  if(Client->canMesh(reqs))
  {
    remus::proto::JobContent content =
    remus::proto::make_JobContent(in);

    remus::proto::JobSubmission sub(reqs,content);

    (*job) = new remus::proto::Job(Client->submitJob(sub));
    return true;
  }
  return false;
}

cmbNucExport::cmbNucExport()
: serverPorts(zmq::socketInfo<zmq::proto::inproc>("export_client_channel"),
              zmq::socketInfo<zmq::proto::inproc>("export_worker_channel")),
  Server(NULL),
  factory(new ExporterWorkerFactory(this, 4)),
  client(NULL)
{
  this->isDone = false;
}

cmbNucExport::~cmbNucExport()
{
  QMutexLocker locker(&Memory);
  delete client;
}

void
cmbNucExport::run( const QStringList &assygenFile,
                   const QString coregenFile,
                   const QString CoreGenOutputFile,
                   const QString assygenFileCylinder,
                   const QString cubitFileCylinder,
                   const QString cubitOutputFileCylinder,
                   const QString coregenFileCylinder,
                   const QString coregenResultFileCylinder )
{
  {
    QMutexLocker locker(&end_control);
    this->isDone = false;
  }
  this->clearJobs();
  if(!this->startUpHelper()) return;

  std::vector<JobHolder*> deps = exportCylinder( assygenFileCylinder, cubitFileCylinder, cubitOutputFileCylinder,
                                                 coregenFileCylinder, coregenResultFileCylinder );

  std::vector<JobHolder*> assy = this->runAssyHelper( assygenFile );

  deps.insert(deps.end(), assy.begin(), assy.end());

  this->runCoreHelper( coregenFile, deps, CoreGenOutputFile );
  processJobs();
  {
    QMutexLocker locker(&end_control);
    this->isDone = true;
  }
}

void cmbNucExport::cancelHelper()
{
  emit currentProcess("  CANCELED");
  emit done();
  emit terminate();
  clearJobs();
  this->deleteServer();
}

void cmbNucExport::failedHelper(QString msg, QString pmsg)
{
  emit errorMessage("ERROR: " + msg );
  emit currentProcess(pmsg + " FAILED");
  emit done();
  clearJobs();
  this->deleteServer();
}

JobHolder* cmbNucExport::makeAssyJob(const QString assygenFile)
{
  QFileInfo fi(assygenFile);
  QString path = fi.absolutePath();
  QString name = fi.completeBaseName();
  QString pass = path + '/' + name;
  JobHolder * assyJob = new JobHolder(path, AssygenExe, name, pass + ".jou");
  jobs_to_do.push_back(assyJob);
  assyJob->label = "Assygen";
  assyJob->itype = "ASSYGEN_IN";
  assyJob->otype = "CUBIT_IN";

  {
    assyJob->in.LibPath = "";
    std::stringstream ss(AssygenLib.toStdString().c_str());
    std::string line;
    while( std::getline(ss, line))
    {
      assyJob->in.LibPath += line + ":";
    }
    QFileInfo libPaths(AssygenExe);
    assyJob->in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
    assyJob->in.LibPath += ":"+ QFileInfo(CubitExe).absolutePath().toStdString();
  }

  return assyJob;
}

std::vector<JobHolder*>
cmbNucExport::runAssyHelper( const QStringList &assygenFile )
{
  std::vector<JobHolder*> dependencies;
  for (QStringList::const_iterator iter = assygenFile.constBegin();
       iter != assygenFile.constEnd(); ++iter)
  {
    QFileInfo fi(*iter);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    QString cubFullFile = path + '/' + name.toLower()+".cub";
    JobHolder * assyJob = makeAssyJob(*iter);

    std::vector<JobHolder*> cjobs =
        runCubitHelper( pass + ".jou", std::vector<JobHolder*>(1, assyJob), cubFullFile );

    dependencies.insert(dependencies.end(), cjobs.begin(), cjobs.end());

  }
  return dependencies;
}


std::vector<JobHolder*>
cmbNucExport::runCubitHelper( const QString cubitFile,
                              std::vector<JobHolder*> depIn,
                              const QString cubitOutputFile )
{
  std::vector<JobHolder*> result;

  if(!cubitFile.isEmpty())
  {
    QFileInfo fi(cubitFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();

    JobHolder * cubitJob = new JobHolder(path, CubitExe, cubitFile, cubitOutputFile);
    cubitJob->label = "Cubit";
    cubitJob->itype = "CUBIT_IN";
    cubitJob->otype = "COREGEN_IN";
    cubitJob->dependencies = depIn;
    jobs_to_do.push_back(cubitJob);
    result.push_back(cubitJob);
  }

  return result;
}

std::vector<JobHolder*>
cmbNucExport::runCoreHelper( const QString coregenFile,
                             std::vector<JobHolder*> depIn,
                             const QString CoreGenOutputFile )
{
  std::vector<JobHolder*> result;
  if(!coregenFile.isEmpty())
  {
    QFileInfo fi(coregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;

    JobHolder * coreJob = new JobHolder(path, CoregenExe, pass, CoreGenOutputFile);
    coreJob->label = "Coregen";
    coreJob->itype = "COREGEN_IN";
    coreJob->otype = "COREGEN_OUT";
    coreJob->dependencies = depIn;
    jobs_to_do.push_back(coreJob);
    result.push_back(coreJob);
    {
      coreJob->in.LibPath = "";
      std::stringstream ss(CoregenLib.toStdString().c_str());
      std::string line;
      while( std::getline(ss, line))
      {
        coreJob->in.LibPath += line + ":";
      }

      QFileInfo libPaths(CoregenExe);
      coreJob->in.LibPath += (libPaths.absolutePath() + ":" + libPaths.absolutePath() + "/../lib").toStdString();
    }
  }
  return result;
}

std::vector<JobHolder*>
cmbNucExport::exportCylinder( const QString assygenFile,
                              const QString cubitFile,
                              const QString cubitOutputFile,
                              const QString coregenFile,
                              const QString coregenResultFile )
{
  std::vector<JobHolder*> result;
  if(assygenFile.isEmpty() || cubitFile.isEmpty() || coregenFile.isEmpty())
  {
    return result;
  }
  JobHolder* assyJob = makeAssyJob(assygenFile);
  std::vector<JobHolder*> tmp = this->runCoreHelper( coregenFile, std::vector<JobHolder*>(1,assyJob),
                                                     coregenResultFile );
  result = runCubitHelper(cubitFile, tmp, cubitOutputFile);
  return result;
}


void cmbNucExport::cancel()
{
  //todo: fix this
  {
    QMutexLocker locker(&Memory);
    for(std::set<cmbNucExporterWorker*>::iterator iter = workers.begin(); iter != workers.end(); ++iter)
    {
      (*iter)->keepGoing = false;
      (*iter)->syncronizer.lock();
      (*iter)->syncronizer.unlock();
    }
    setKeepGoing(false);
  }
  emit cancelled();
}

void cmbNucExport::finish()
{
  this->deleteServer();
  clearJobs();
  emit progress(100);
  emit currentProcess("Finished");
  emit done();
  emit successful();
}

bool cmbNucExport::keepGoing() const
{
  QMutexLocker locker(&Mutex);
  return KeepGoing;
}

void cmbNucExport::setKeepGoing(bool b)
{
  QMutexLocker locker(&Mutex);
  KeepGoing = b;
}

void cmbNucExport::deleteServer()
{
  QMutexLocker locker(&ServerProtect);
  { //make sure workers are done
    QMutexLocker memlocker(&Memory);
    for(std::set<cmbNucExporterWorker*>::iterator iter = workers.begin(); iter != workers.end(); ++iter)
    {
      (*iter)->keepGoing = false;
      (*iter)->syncronizer.lock();
      (*iter)->syncronizer.unlock();
    }
  }
  workers.clear();
  qDebug() << "stop brocking";
  if(this->Server != NULL) this->Server->stopBrokering();
  delete this->client;
  this->client = NULL;
  qDebug() << "deleting server";
  delete this->Server;
  this->Server = NULL;
}

bool cmbNucExport::startUpHelper()
{
  {
    QMutexLocker locker(&ServerProtect);
    if(Server != NULL)
    {
      emit errorMessage("Currently running a process");
      emit currentProcess("  Please wait until previous process completes");
      return false;
    }
  }
  this->setKeepGoing(true);
  emit currentProcess("  Started setting up");
  //pop up progress bar
  emit progress(static_cast<int>(1));
  //start server
  //create a default server with the factory
  {
    QMutexLocker locker(&ServerProtect);
    Server = new remus::server::Server(serverPorts, factory);
  }

  //start accepting connections for clients and workers
  emit currentProcess("  Starting server");
  bool valid;
  {
    QMutexLocker locker(&ServerProtect);
    valid = Server->startBrokering(remus::Server::NONE);
    remus::client::ServerConnection conn = remus::client::make_ServerConnection(serverPorts.client().endpoint());
    conn.context(serverPorts.context());
    client = new cmbNucExporterClient(conn);
  }
  if( !valid )
  {
    QMutexLocker locker(&ServerProtect);
    emit errorMessage("failed to start server");
    emit currentProcess("  failed to start server");
    emit done();
    delete Server;
    delete client;
    Server = NULL;
    client = NULL;
    return false;
  }

  {
    QMutexLocker locker(&ServerProtect);
    Server->waitForBrokeringToStart();
  }
  emit progress(2);

  return true;
}

void cmbNucExport::setAssygen(QString assygenExe,QString assygenLib)
{
  this->AssygenExe = assygenExe;
  this->AssygenLib = assygenLib;
}

void cmbNucExport::setCoregen(QString coregenExe,QString coregenLib)
{
  this->CoregenExe = coregenExe;
  this->CoregenLib = coregenLib;
}

void cmbNucExport::setNumberOfProcessors(int v)
{
  this->factory->setMaxThreadCount(v);
}

void cmbNucExport::setCubit(QString cubitExe)
{
  this->CubitExe = cubitExe;
}

void cmbNucExport::registerWorker(cmbNucExporterWorker* w)
{
  QMutexLocker locker(&Memory);
  workers.insert(w);
}

void cmbNucExport::workerDone(cmbNucExporterWorker* w)
{
  QMutexLocker locker(&Memory);
  workers.erase(w);
}

void cmbNucExport::clearJobs()
{
  for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
  {
    delete jobs_to_do[i];
    jobs_to_do[i] = NULL;
  }
  jobs_to_do.clear();
}

void cmbNucExport::processJobs()
{
  QEventLoop el;
  emit currentProcess("  Generating Mesh");
  double count = 2;
  while(true)
  {
    if(!keepGoing())
    {
      cancelHelper();
      return;
    }
    el.processEvents();
    bool all_finish = true;
    for(unsigned int i = 0; i < jobs_to_do.size(); ++i)
    {
      if(jobs_to_do[i]->done) continue;
      else if(jobs_to_do[i]->running )
      {
        ServerProtect.lock();
        remus::proto::JobStatus jobState = client->jobStatus(jobs_to_do[i]->job);
        ServerProtect.unlock();

        if(jobState.good())
        {
          all_finish = false;
        }
        else if(jobState.finished())
        {
          if(!jobs_to_do[i]->done)
          {
            emit progress(static_cast<int>((count++)/(jobs_to_do.size()+2)*100));
            emit fileDone();
          }
          jobs_to_do[i]->done = true;
        }
        else if(jobState.status() == remus::INVALID_STATUS)
        {
          failedHelper("Remus ERROR", " Remus Invalid Status");
          return;
        }
        else if(jobState.status() == remus::FAILED)
        {
          failedHelper("Remus ERROR", " Failed to mesh");
          return;
        }
        else if(jobState.status() == remus::EXPIRED)
        {
          failedHelper("Remus ERROR", " Remus Expired");
          return;
        }
        else
        {
          failedHelper("Remus ERROR", " Remus did not finish but was not good");
          return;
        }
      }
      else
      {
        all_finish = false;
        bool taf = true;
        for(unsigned int j = 0; j < this->jobs_to_do[i]->dependencies.size(); ++j)
        {
          taf = taf && this->jobs_to_do[i]->dependencies[j]->done;
        }
        if(taf)
        {
          QMutexLocker locker(&ServerProtect);
          this->jobs_to_do[i]->running = true;
          qDebug() << "i is starting" << i;
          bool r = client->getOutput( jobs_to_do[i]->label, jobs_to_do[i]->itype,
                                      jobs_to_do[i]->otype, jobs_to_do[i]->in,
                                      &(jobs_to_do[i]->job));
          if(!r)
          {
            failedHelper("Remus ERROR", " Remus does not support the job");
            return;
          }
        }
      }
    }
    if(all_finish) break;
  }
  this->finish();
}

remus::worker::ServerConnection
cmbNucExport::make_ServerConnection()
{
  remus::worker::ServerConnection conn = remus::worker::make_ServerConnection(serverPorts.worker().endpoint());
  conn.context(serverPorts.context());
  return conn;
}

void
cmbNucExport::waitTillDone()
{
  QEventLoop el;
  {
    QMutexLocker locker(&end_control);
  }
  while( !this->isDone )
  {
    el.processEvents();
  }
  {
    this->isDone = false;
  }
}

#endif //#ifndef cmbNucExport_cxx

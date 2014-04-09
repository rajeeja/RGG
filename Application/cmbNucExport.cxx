#ifndef cmbNucExport_cxx
#define cmbNucExport_cxx //incase this is included for template instant

#include <remus/client/Client.h>

#include <remus/worker/Worker.h>

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>
#include <remus/common/MeshTypes.h>
#include <remus/common/ExecuteProcess.h>
#include <remus/client/JobResult.h>

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QDir>
#include <QMutexLocker>
#include <QEventLoop>
#include <QThread>

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

#include <iostream>

#include <iostream>

#include "cmbNucExport.h"

struct ExporterInput
{
  std::string ExeDir;
  std::string Function;
  std::string FileArg;
  operator std::string(void) const
  {
    std::stringstream ss;
    ss << ExeDir << ';' << FileArg << ";" <<Function << ";";
    return ss.str();
  }
  ExporterInput(std::string exeDir, std::string fun, std::string file)
  :ExeDir(exeDir), Function(fun),FileArg(file)
  {  }
  ExporterInput(QString exeDir, QString fun, QString file)
  :ExeDir(exeDir.trimmed().toStdString()), Function(fun.trimmed().toStdString()),
   FileArg(file.trimmed().toStdString())
  {  }
  ExporterInput(const remus::worker::Job& job)
  {
    std::stringstream ss(job.details());
    getline(ss, ExeDir,   ';');
    getline(ss, FileArg,  ';');
    getline(ss, Function, ';');
  }
};

struct ExporterOutput
{
  bool Valid;
  std::string Result;
  ExporterOutput():Valid(false){}
  ExporterOutput(const remus::client::JobResult& job, bool valid=true)
  : Valid(valid), Result(job.Data)
  {}
  operator std::string(void) const
  { return Result; }
};

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
class cmbNucExporterClient
{
public:
  cmbNucExporterClient(std::string server = "");
  ExporterOutput getOutput(ExporterInput const& in);
private:
  remus::client::ServerConnection Connection;
  remus::Client * Client;
};

typedef cmbNucExporterClient<remus::meshtypes::SceneFile,remus::meshtypes::Model> AssygenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Mesh3D,remus::meshtypes::Mesh3D> CoregenExporter;
typedef cmbNucExporterClient<remus::meshtypes::Model,remus::meshtypes::Mesh3D> CubitExporter;

cmbNucExporterWorker *
cmbNucExporterWorker
::AssygenWorker(std::vector<std::string> extra_args,
                remus::worker::ServerConnection const& connection)
{
  cmbNucExporterWorker * r =
         new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::SceneFile(),
                                                            remus::meshtypes::Model()),
                                  extra_args, connection );
  r->Name = "Assygen";
  return r;
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CoregenWorker(std::vector<std::string> extra_args,
                remus::worker::ServerConnection const& connection)
{
  cmbNucExporterWorker * r =
        new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::Mesh3D(),
                                                            remus::meshtypes::Mesh3D()),
                                  extra_args, connection );
  r->Name = "CoreGen";
  return r;
}

cmbNucExporterWorker *
cmbNucExporterWorker
::CubitWorker(std::vector<std::string> extra_args,
             remus::worker::ServerConnection const& connection)
{
  cmbNucExporterWorker * r =
         new cmbNucExporterWorker(remus::common::MeshIOType(remus::meshtypes::Model(),
                                                            remus::meshtypes::Mesh3D()),
                                  extra_args, connection );
  r->Name = "Cubit";
  return r;
}

cmbNucExporterWorker
::cmbNucExporterWorker( remus::common::MeshIOType miot,
                        std::vector<std::string> extra_args,
                        remus::worker::ServerConnection const& connection)
:remus::worker::Worker(miot, connection),
 ExtraArgs(extra_args)
{
}

void cmbNucExporterWorker::run()
{
  {
    QMutexLocker locker(&Mutex);
    StillRunning = true;
  }
  while(stillRunning())
  {
    remus::worker::Job job = this->getJob();

    if(!job.valid())
    {
      switch(job.validityReason())
      {
        case remus::worker::Job::INVALID:
          emit errorMessage("JOB NOT VALID");
          continue;
        case remus::worker::Job::TERMINATE_WORKER:
          {
          qDebug() << this->Name.c_str() << "Told to terminate";
          this->stop();
          }
          continue;
      }
    }

    ExporterInput input(job);

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
    qDebug() <<  this->Name.c_str() << "before create exe process";

    remus::common::ExecuteProcess* process = new remus::common::ExecuteProcess( input.Function, args);
    qDebug() <<  QString((input.Function + "  " + input.FileArg).c_str());

    //actually launch the new process
    process->execute(remus::common::ExecuteProcess::Attached);
    qDebug() <<  this->Name.c_str() << "processed launched";

    //Wait for finish
    if(pollStatus(process, job))
    {
      qDebug() <<  this->Name.c_str() << "poll success";
      remus::worker::JobResult results(job.id(),"DUMMY FOR NOW;");
      this->returnMeshResults(results);
    }
    else
    {
      qDebug() <<  this->Name.c_str() << "poll fail";
      remus::worker::JobStatus status(job.id(),remus::FAILED);
      updateStatus(status);
    }
    QDir::setCurrent( current );
    qDebug() <<  this->Name.c_str() << "deleting process";
    delete process;
    qDebug() <<  this->Name.c_str() << "done deleting process";
  }
  qDebug() << this->Name.c_str() << "Worker finished running";
}

void cmbNucExporterWorker::stop()
{
  QMutexLocker locker(&Mutex);
  qDebug() << this->Name.c_str() << "Worker stopping";
  StillRunning = false;
}

bool cmbNucExporterWorker::stillRunning()
{
  QMutexLocker locker(&Mutex);
  return StillRunning;
}

bool cmbNucExporterWorker
::pollStatus( remus::common::ExecuteProcess* process,
              const remus::worker::Job& job)
{
  typedef remus::common::ProcessPipe ProcessPipe;

  //poll on STDOUT and STDERRR only
  bool validExection=true;
  remus::worker::JobStatus status(job.id(),remus::IN_PROGRESS);
  while(process->isAlive()&& validExection && this->stillRunning())
  {
    //poll till we have a data, waiting for-ever!
    ProcessPipe data = process->poll(2);
    if(data.type == ProcessPipe::STDOUT)
    {
      //we have something on the output pipe
      status.Progress.setMessage("bob");
      this->updateStatus(status);
    }
  }
  if(process->isAlive())
  {
    process->kill();
  }

  //verify we exited normally, not segfault or numeric exception
  validExection &= process->exitedNormally();

  if(!validExection)
  {
    return false;
  }
  return true;
}

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::cmbNucExporterClient(std::string server)
{
  if ( !server.empty())
    {
    Connection = remus::client::make_ServerConnection(server);
    }
  Client = new remus::Client(Connection);
}

template<class JOB_REQUEST_IN, class JOB_REQUEST_OUT>
ExporterOutput
cmbNucExporterClient<JOB_REQUEST_IN, JOB_REQUEST_OUT>::getOutput(ExporterInput const& in)
{
  JOB_REQUEST_IN in_type;
  JOB_REQUEST_OUT out_type;
  ExporterOutput eo;
  remus::client::JobRequest request(in_type, out_type, in);
  unsigned int i = 0;
  while( !Client->canMesh(request) && i++ < 60 )
    {
    qDebug() << "Can mesh returns false";
    Thread::msleep(1000);
    JOB_REQUEST_IN tin;
    JOB_REQUEST_OUT tout;
    request = remus::client::JobRequest(tin, tout, in);
    }
  if(Client->canMesh(request))
    {
    remus::client::Job job = Client->submitJob(request);
    remus::client::JobStatus jobState = Client->jobStatus(job);

    //wait while the job is running
    QEventLoop el;
    while(jobState.good())
      {
      el.processEvents();
      jobState = Client->jobStatus(job);
      };

    if(jobState.finished())
      {
      remus::client::JobResult result = Client->retrieveResults(job);
      return ExporterOutput(result);
      }
    else if(jobState.Status == remus::INVALID_STATUS)
      {
      eo.Result = " Remus Invalid Status";
      }
    else if(jobState.Status == remus::FAILED)
      {
      eo.Result = " Remus Failed";
      }
    else if(jobState.Status == remus::EXPIRED)
      {
      eo.Result = " Remus Expired";
      }
    else
      {
      eo.Result = " Remus did not finish but was not good";
      }
    }
  else
    {
    eo.Result = " Remus Server does not support the job request";
    }
  return eo;
}

void cmbNucExport::run( const QString assygenExe,
                        const QStringList &assygenFile,
                        const QString cubitExe,
                        const QString coregenExe,
                        const QString coregenFile,
                        const QString CoreGenOutputFile )
{
  qDebug() << "Output file: " << CoreGenOutputFile;
  this->setKeepGoing(true);
  emit currentProcess("  Started setting up");
  //pop up progress bar
  double total_number_of_file = 2.0*assygenFile.count() + 6;
  double current = 0;
  emit progress(static_cast<int>(current/total_number_of_file*100));
  //start server
  remus::server::WorkerFactory factory;
  factory.setMaxWorkerCount(0);

  //create a default server with the factory
  remus::server::Server b(factory);

  //start accepting connections for clients and workers
  emit currentProcess("  Starting server");
  bool valid = b.startBrokering(remus::Server::NONE);
  if( !valid )
  {
    emit errorMessage("failed to start server");
    emit currentProcess("  failed to start server");
    emit done();
    return;
  }

  b.waitForBrokeringToStart();
  current++;
  emit progress(static_cast<int>(current/total_number_of_file*100));

  emit currentProcess("  Starting Workers");
  this->constructWorkers();
  current += 3;
  emit progress(static_cast<int>(current/total_number_of_file*100));

  //Send files
  AssygenExporter ae;
  CubitExporter ce;
  for (QStringList::const_iterator i = assygenFile.constBegin();
       i != assygenFile.constEnd(); ++i)
  {
    if(!keepGoing())
    {
      emit currentProcess("  CANCELED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    QFileInfo fi(*i);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    emit currentProcess("  assygen " + name);
    QFile::remove(pass + ".jou");
    ExporterOutput lr = ae.getOutput(ExporterInput(path, assygenExe, pass));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!lr.Valid)
    {
      emit errorMessage("Assygen failed");
      emit currentProcess("  assygen " + name + " FAILED" + lr.Result.c_str());
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    if(!QFileInfo(pass + ".jou").exists())
    {
      emit errorMessage("ERROR: Assygen failed to generate jou file");
      emit currentProcess("  "+name + ".jou does not exist FAILED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    if(!keepGoing())
    {
      emit currentProcess("  CANCELED");
      emit done();
      return;
    }

    emit currentProcess("  cubit " + name + ".jou");
    QFile::remove(pass + ".cub");
    lr = ce.getOutput(ExporterInput(path, cubitExe, pass + ".jou"));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!lr.Valid)
    {
      emit errorMessage("Cubit failed");
      emit currentProcess("  cubit " + name + ".jou FAILED" + lr.Result.c_str());
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    if(!QFileInfo(pass + ".cub").exists())
    {
      emit errorMessage("ERROR: cubit failed to generate cubit file");
      emit currentProcess("  " + name + ".cub does not exist FAILED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    if(!keepGoing())
    {
      emit currentProcess("  CANCELED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
  }
  {
    if(!keepGoing())
    {
      emit currentProcess("  CANCELED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    QFile::remove(CoreGenOutputFile);
    QFileInfo fi(coregenFile);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName();
    QString pass = path + '/' + name;
    emit currentProcess("  coregen " + name + ".inp");
    CoregenExporter coreExport;
    ExporterOutput r = coreExport.getOutput(ExporterInput(path, coregenExe, pass));
    current++;
    emit progress(static_cast<int>(current/total_number_of_file*100));
    if(!r.Valid)
    {
      emit errorMessage("ERROR: Curegen failed");
      emit currentProcess("  running coregen " + name + ".inp FAILED returned failure mode" + r.Result.c_str());
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
    qDebug() << "testing to see if " << CoreGenOutputFile << "exists";
    if(QFileInfo(CoreGenOutputFile).exists())
    {
      emit sendCoreResult(CoreGenOutputFile);
    }
    else
    {
      qDebug() << CoreGenOutputFile;
      emit errorMessage("ERROR: Curegen failed to generate model file");
      emit currentProcess("  coregen did not generage a model file FAILED");
      emit done();
      b.stopBrokering();
      this->deleteWorkers();
      return;
    }
  }
  b.stopBrokering();
  this->deleteWorkers();
  current++;
  emit progress(static_cast<int>(current/total_number_of_file*100));
  emit currentProcess("Finished");
  emit done();
  emit successful();
}

void cmbNucExport::cancel()
{
  {
    QMutexLocker locker(&Memory);
    setKeepGoing(false);
    if(assygenWorker != NULL) assygenWorker->stop();
    if(coregenWorker != NULL) coregenWorker->stop();
    if(cubitWorker != NULL) cubitWorker->stop();
  }
  this->deleteWorkers();
  emit cancelled();
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

void cmbNucExport::constructWorkers()
{
  QMutexLocker locker(&Memory);
  assygenWorker = cmbNucExporterWorker::AssygenWorker();
  assygenWorker->moveToThread(&(WorkerThreads[0]));
  connect( this, SIGNAL(startWorkers()),
           assygenWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          assygenWorker, SLOT(stop()));
  coregenWorker = cmbNucExporterWorker::CoregenWorker();
  coregenWorker->moveToThread(&(WorkerThreads[1]));
  connect( this, SIGNAL(startWorkers()),
           coregenWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          coregenWorker, SLOT(stop()));
  std::vector<std::string> args;
  args.push_back("-nographics");
  args.push_back("-batch");
  cubitWorker = cmbNucExporterWorker::CubitWorker(args);
  cubitWorker->moveToThread(&WorkerThreads[2]);
  connect( this, SIGNAL(startWorkers()),
          cubitWorker, SLOT(start()));
  connect( this, SIGNAL(endWorkers()),
          cubitWorker, SLOT(stop()));
  WorkerThreads[0].start();
  WorkerThreads[1].start();
  WorkerThreads[2].start();
  emit startWorkers();
}

void cmbNucExport::deleteWorkers()
{
  QMutexLocker locker(&Memory);
  WorkerThreads[0].quit();
  WorkerThreads[1].quit();
  WorkerThreads[2].quit();
  WorkerThreads[0].wait();
  WorkerThreads[1].wait();
  WorkerThreads[2].wait();
  delete assygenWorker;
  assygenWorker = NULL;
  delete coregenWorker;
  coregenWorker = NULL;
  delete cubitWorker;
  cubitWorker = NULL;
}


#endif //#ifndef cmbNucExport_cxx

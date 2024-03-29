#ifndef cmbNucExport_H
#define cmbNucExport_H

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <QObject>
#include <QThread>
#include <QMutex>

#include <remus/client/ServerConnection.h>
#include <remus/worker/Worker.h>
#include <remus/server/Server.h>

namespace remus
{
namespace common
{
class ExecuteProcess;
}
namespace server
{
class Server;
class WorkerFactory;
}
}

class cmbNucExporterWorker;
class ExporterWorkerFactory;
class cmbNucExporterClient;
struct JobHolder;

class RunnableConnection : public QObject
{
  Q_OBJECT
public:
  RunnableConnection():terminate(false)
  {}
  bool terminate;
  void sendErrorMessage(QString s)
  { emit errorMessage(s); }
  void sendCurrentMessage(QString s)
  { emit currentMessage(s);}
  void sendDoNextStep(QString outputFile, int index)
  { emit doNextStep(outputFile, index); }
public slots:
  void terminateProcess()
  {
    terminate = true;
  }
signals:
  void errorMessage( QString );
  void currentMessage( QString );
  void doNextStep(QString, int);
};

class cmbNucExport : public QObject
{
  Q_OBJECT
  QThread workerThread;
public:
  cmbNucExport();
  ~cmbNucExport();
  void setKeepGoing(bool);
  void setAssygen(QString assygenExe,QString assygenLib);
  void setCoregen(QString coregenExe,QString coregenLib);
  void setNumberOfProcessors(int v);
  void setCubit(QString cubitExe);
  void registerWorker(cmbNucExporterWorker*);
  void workerDone(cmbNucExporterWorker*);
  remus::worker::ServerConnection make_ServerConnection();
  void waitTillDone();
public slots:
  void run( const QStringList &assygenFile,
            const QString coregenFile,
            const QString CoreGenOutputFile,
            const QString assygenFileCylinder,
            const QString cubitFileCylinder,
            const QString cubitOutputFileCylinder,
            const QString coregenFileCylinder,
            const QString coregenResultFileCylinder );
  void cancel();
signals:
  void done();
  void successful();
  void cancelled();
  void terminate();
  void sendCoreResult(QString);
  void progress(int);
  void currentProcess(QString);
  void errorMessage( QString );
  void fileDone();
  void statusMessage(QString);
protected:
signals:
  void startWorkers();
  void endWorkers();

private:
  std::vector<JobHolder*> runAssyHelper( const QStringList &assygenFile);
  std::vector<JobHolder*> runCubitHelper(const QString cubitFile,
                                         std::vector<JobHolder*> debIn,
                                         const QString cubitOutputFile);
  std::vector<JobHolder*> runCoreHelper( const QString coregenFile,
                                         std::vector<JobHolder*> debIn,
                                         const QString CoreGenOutputFile );
  std::vector<JobHolder*> exportCylinder( const QString assygenFile,
                                          const QString cubitFile,
                                          const QString cubitOutputFile,
                                          const QString coregenFile,
                                          const QString coregenResultFile );
  JobHolder* makeAssyJob(const QString assygenFile);

  void finish();
  void cancelHelper();
  void failedHelper(QString msg, QString pmsg);
  bool startUpHelper();
  bool keepGoing() const;
  bool KeepGoing;
  void deleteServer();
  mutable QMutex Mutex, Memory, ServerProtect;
  remus::server::ServerPorts serverPorts;
  remus::server::Server * Server;
  remus::worker::ServerConnection ServerConnection;
  boost::shared_ptr<ExporterWorkerFactory> factory;
  QString AssygenExe, AssygenLib;
  QString CoregenExe, CoregenLib;
  QString CubitExe;
  std::set<cmbNucExporterWorker*> workers;

  std::vector<JobHolder*> jobs_to_do;
  cmbNucExporterClient * client;
  void clearJobs();
  void processJobs();
  mutable QMutex end_control;
  bool isDone;
};

#endif //cmbNucExport_H

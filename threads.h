// Template, UU version
// IGAD/NHTV/UU - Jacco Bikker - 2006-2018

#pragma once

class Thread 
{
public:
	Thread() { m_hThread = 0; }
	unsigned long* handle() { return m_hThread; }
	void start();
	virtual void run() {};
	void sleep(long ms);
	void suspend();
	void resume();
	void kill();
	void stop();
	void setPriority( int p );
	void SetName( char* _Name );
private:
	unsigned long* m_hThread;
	static const int P_ABOVE_NORMAL;
	static const int P_BELOW_NORMAL;
	static const int P_HIGHEST;
	static const int P_IDLE;
	static const int P_LOWEST;
	static const int P_NORMAL;
	static const int P_CRITICAL;
};
extern "C" { unsigned int sthread_proc( void* param ); }

namespace Tmpl8 {

class Job
{
public:
	virtual void Main() = 0;
protected:
	friend class JobThread;
	void RunCodeWrapper();
};

class JobThread
{
public:
	void CreateAndStartThread( unsigned int threadId );
	void WaitForThreadToStop();
	void Go();
	void BackgroundTask();
	HANDLE m_GoSignal, m_ThreadHandle;
	int m_ThreadID;
};

class JobManager	// singleton class!
{
protected:
	JobManager( unsigned int numThreads );
public:
	~JobManager();
	static void CreateJobManager( unsigned int numThreads );
	static JobManager* GetJobManager() { return m_JobManager; }
	void AddJob2( Job* a_Job );
	unsigned int GetNumThreads() { return m_NumThreads; }
	void RunJobs();
	void ThreadDone( unsigned int n );
	int MaxConcurrent() { return m_NumThreads; }
protected:
	friend class JobThread;
	Job* GetNextJob();
	Job* FindNextJob();
	static JobManager* m_JobManager;
	Job* m_JobList[64];
	CRITICAL_SECTION m_CS;
	HANDLE m_ThreadDone[4];
	unsigned int m_NumThreads, m_JobCount;
	JobThread* m_JobThreadList;
};

}; // namespace Tmpl8
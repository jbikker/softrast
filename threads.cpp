// Template, UU version
// IGAD/NHTV/UU - Jacco Bikker - 2006-2018

#include "precomp.h"

using namespace Tmpl8;

const int Thread::P_ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL;
const int Thread::P_BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL;
const int Thread::P_HIGHEST = THREAD_PRIORITY_HIGHEST;
const int Thread::P_IDLE = THREAD_PRIORITY_IDLE;
const int Thread::P_LOWEST = THREAD_PRIORITY_LOWEST;
const int Thread::P_NORMAL = THREAD_PRIORITY_NORMAL;
const int Thread::P_CRITICAL = THREAD_PRIORITY_TIME_CRITICAL;

unsigned int sthread_proc( void* param ) { Thread* tp = (Thread*)param; tp->run(); return 0; }

void Thread::sleep( long ms ) { Sleep( ms ); }
void Thread::setPriority( int tp ) { SetThreadPriority( m_hThread, tp ); }
void Thread::suspend() { SuspendThread( m_hThread ); }
void Thread::resume() { ResumeThread( m_hThread ); }

void Thread::start() 
{
	DWORD tid = 0;	
	m_hThread = (unsigned long*)CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)sthread_proc, (Thread*)this, 0, &tid );
	setPriority( Thread::P_NORMAL );
}

void Thread::kill()
{
	TerminateThread( m_hThread, 0 );
	CloseHandle( m_hThread );
	m_hThread = NULL;
}

void Thread::stop() 
{
	if (m_hThread == NULL) return;	
	WaitForSingleObject( m_hThread, INFINITE );
	CloseHandle( m_hThread );
	m_hThread = NULL;
}

void Thread::SetName( char* _Name )
{
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;		// must be 0x1000
		LPCSTR szName;		// pointer to name (in user addr space)
		DWORD dwThreadID;	// thread ID (-1=caller thread)
		DWORD dwFlags;		// reserved for future use, must be zero
	} THREADNAME_INFO;
	DWORD dwThreadID = GetThreadId( m_hThread );
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = _Name;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
	if (::IsDebuggerPresent()) RaiseException( 0x406D1388, 0, sizeof( info ) / sizeof( ULONG_PTR ), (ULONG_PTR*)&info );
}

DWORD JobThreadProc( LPVOID lpParameter )
{
	JobThread* JobThreadInstance = (JobThread*) lpParameter;
	JobThreadInstance->BackgroundTask();
	return 0;
}

void JobThread::CreateAndStartThread( unsigned int threadId )
{
	m_GoSignal = CreateEvent( 0, FALSE, FALSE, 0 );
	m_ThreadHandle = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE) &JobThreadProc, (LPVOID) this, 0, 0 );
	m_ThreadID = threadId;
}
void JobThread::BackgroundTask()
{
	while (1)
	{
		WaitForSingleObject( m_GoSignal, INFINITE );
		while (1)
		{
			Job* job = JobManager::GetJobManager()->GetNextJob();
			if (!job) 
			{
				JobManager::GetJobManager()->ThreadDone( m_ThreadID );
				break;
			}
			job->RunCodeWrapper();
		}
	}
}

void JobThread::Go()
{
	SetEvent( m_GoSignal );
}

void Job::RunCodeWrapper()
{
	Main();
}
JobManager* JobManager::m_JobManager = 0;

JobManager::JobManager( unsigned int threads ) : m_NumThreads( threads )
{
	InitializeCriticalSection( &m_CS );
}

JobManager::~JobManager()
{
	DeleteCriticalSection( &m_CS );
}

void JobManager::CreateJobManager( unsigned int numThreads )
{
	m_JobManager = new JobManager( numThreads );
	m_JobManager->m_JobThreadList = new JobThread[numThreads];
	for ( unsigned int i = 0 ; i < numThreads; i++ ) 
	{
		m_JobManager->m_JobThreadList[i].CreateAndStartThread( i );
		m_JobManager->m_ThreadDone[i] = CreateEvent( 0, FALSE, FALSE, 0 );
	}
	m_JobManager->m_JobCount = 0;
}

void JobManager::AddJob2( Job* a_Job )
{
	m_JobList[m_JobCount++] = a_Job;
}

Job* JobManager::GetNextJob()
{
	Job* job = 0;
	EnterCriticalSection( &m_CS );
	if (m_JobCount > 0) job = m_JobList[--m_JobCount];
	LeaveCriticalSection( &m_CS );
	return job;
}

void JobManager::RunJobs()
{
	for ( unsigned int i = 0; i < m_NumThreads; i++ ) 
	{
		m_JobThreadList[i].Go();
	}
	WaitForMultipleObjects( m_NumThreads, m_ThreadDone, TRUE, INFINITE );
}

void JobManager::ThreadDone( unsigned int n ) 
{ 
	SetEvent( m_ThreadDone[n] ); 
}

// EOF
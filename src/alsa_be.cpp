#include <signal.h>

#include <glog/logging.h>

#include "AlsaBackend.hpp"

using std::exception;
using std::runtime_error;
using std::unique_ptr;

unique_ptr<AlsaBackend> alsaBackend;

void terminate(int sig, siginfo_t *info, void *ptr)
{
	alsaBackend->stop();
}

void segHandler(int sig)
{
	LOG(FATAL) << "Unknown error!";
}

void registerTerminate()
{
	struct sigaction action;

	action.sa_sigaction = terminate;
	sigfillset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &action, NULL) < 0)
	{
		throw runtime_error(strerror(errno));
	}

	if (sigaction(SIGTERM, &action, NULL) < 0)
	{
		throw runtime_error(strerror(errno));
	}

	signal(SIGSEGV, segHandler);
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);

	try
	{
		registerTerminate();

		alsaBackend.reset(new AlsaBackend(0, "audio"));

		alsaBackend->run();
	}
	catch(const exception& e)
	{
		LOG(ERROR) << e.what();
	}
	catch(...)
	{
		LOG(ERROR) << "Unknown error";
	}

	return 0;
}

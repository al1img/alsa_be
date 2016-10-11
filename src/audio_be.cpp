#include <signal.h>

#include <glog/logging.h>

#include "BEAudio.hpp"

using std::exception;
using std::runtime_error;
using std::unique_ptr;

unique_ptr<BEAudio> beAudio;

void terminate (int sig, siginfo_t *info, void *ptr)
{
	beAudio->stop();
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
}

int main(int argc, char *argv[])
{
	google::InitGoogleLogging(argv[0]);

	try
	{
		registerTerminate();

		beAudio.reset(new BEAudio(0, "audio"));

		beAudio->run();
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

/*
 * FEHandlerBase.h
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#ifndef INCLUDE_FEHANDLERBASE_HPP_
#define INCLUDE_FEHANDLERBASE_HPP_

#include <exception>
#include <string>

class BEBase;

class FEHandlerException : public std::exception
{
public:
	FEHandlerException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class FEHandlerBase
{
public:
	FEHandlerBase(int domId, const BEBase& be);
	virtual ~FEHandlerBase();

private:
	int mDomId;
	const BEBase& mBe;

	std::string mXsBePath;
	std::string mXsFePath;
	std::string mXsRemovePath;

	const std::string getXsBePath();
	const std::string getXsFePath();
	const std::string getXsRemovePath();
};


#endif /* INCLUDE_FEHANDLERBASE_HPP_ */

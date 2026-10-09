/* ****************************************** */
/*   Exception.hpp                         */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/05 15:59:54 by Luc      */
/*   Updated: 2026/10/05 15:59:54 by Luc      */
/* ****************************************** */

#pragma once

#include <exception>
#include <string>
using namespace std;

class glfwError : public exception {
	public:
		explicit glfwError(const std::string& m) : _msg(m) {}
		const char* what() const noexcept override { return _msg.c_str(); }
	private :
		string _msg;
};

class openGlError : public exception {
	public:
		explicit openGlError(const std::string& m) : _msg(m) {}
		const char* what() const noexcept override { return _msg.c_str(); }
	private :
		string _msg;
};
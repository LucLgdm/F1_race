/* ****************************************** */
/*   Activation.hpp                           */
/*                                            */
/*   By: Luc <lucdemercey@gmail.com>          */
/*                                            */
/*   Created: 2026/10/11 22:23:37 by Luc      */
/*   Updated: 2026/10/11 22:23:37 by Luc      */
/* ****************************************** */

#pragma once

#include <algorithm>
#include <cmath>

#include "Exception.hpp"

enum class Activation {
	Linear,
	ReLU,
	Tanh
};

inline float activate(float z, Activation activation) {
	switch (activation) {
		case Activation::Linear:
			return z;
		case Activation::ReLU:
			return std::max(0.0f, z);
		case Activation::Tanh:
			return std::tanh(z);
		default:
			throw neuralError("Unknown activation function.");
	}
}

inline float activationDerivative(float z, Activation activation) {
	switch (activation) {
		case Activation::Linear:
			return 1.0f;
		case Activation::ReLU:
			return z > 0.0f ? 1.0f : 0.0f;
		case Activation::Tanh:
		{
			const float output = std::tanh(z);
			return 1.0f - output * output;
		}
	}
	throw neuralError("Unknown activation function: derivate");
}

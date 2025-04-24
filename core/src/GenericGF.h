/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*/
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GenericGFPoly.h"
#include "ZXConfig.h"

#include <stdexcept>
#include <vector>

namespace ZXing {

/**
* <p>This class contains utility methods for performing mathematical operations over
* the Galois Fields. Operations use a given primitive polynomial in calculations.</p>
*
* <p>Throughout this package, elements of the GF are represented as an {@code int}
* for convenience and speed (but at the cost of memory).
* </p>
*
* @author Sean Owen
* @author David Olivier
*/
class GenericGF
{
protected:
	const int _size;
	int _generatorBase;
	std::vector<short> _expTable;
	std::vector<short> _logTable;

	/**
	* Create a representation of GF(size) using the given primitive polynomial.
	*
	* @param primitive irreducible polynomial whose coefficients are represented by
	*  the bits of an int, where the least-significant bit represents the constant
	*  coefficient
	* @param size the size of the field (m = log2(size) is called the word size of the encoding)
	* @param b the factor b in the generator polynomial can be 0- or 1-based
	*  (g(x) = (x+a^b)(x+a^(b+1))...(x+a^(b+2t-1))).
	*  In most cases it should be 1, but for QR code it is 0.
	*/
	GenericGF(int primitive, int size, int b);

	int fast_mod(const int input, const int ceil) const {
		// avoid using the '%' modulo operator => ReedSolomon computation is more than twice as fast
		// see also https://stackoverflow.com/a/33333636/2088798
		return input < ceil ? input : input - ceil;
	};

public:
	virtual ~GenericGF() = default;

	static const GenericGF& AztecData12();
	static const GenericGF& AztecData10();
	static const GenericGF& AztecData6();
	static const GenericGF& AztecParam();
	static const GenericGF& QRCodeField256();
	static const GenericGF& DataMatrixField256();
	static const GenericGF& AztecData8();
	static const GenericGF& MaxiCodeField64();
	static const GenericGF& HanXinField256();
	static const GenericGF& HanXinFuncInfo();

	/**
	* @return 2 (GF(2**n)) or 3 (GF(p)) to the power of a in GF(size)
	*/
	int exp(int a) const {
		return _expTable.at(a);
	}

	/**
	* @return base 2/3 log of a in GF(size)
	*/
	int log(int a) const {
		if (a == 0) {
			throw std::invalid_argument("a == 0");
		}
		return _logTable.at(a);
	}

	/**
	* @return multiplicative inverse of a
	*/
	int inverse(int a) const {
		return _expTable[_size - log(a) - 1];
	}

	/**
	* Implements addition -- same as subtraction in GF(2**size).
	*
	* @return sum of a and b
	*/
	virtual int add(int a, int b) const noexcept {
		return a ^ b;
	}

	/**
	* Implements subtraction -- same as addition in GF(2**size).
	*
	* @return difference of a and b
	*/
	virtual int subtract(int a, int b) const noexcept {
		return a ^ b;
	}

	/**
	* @return product of a and b in GF(size)
	*/
	int multiply(int a, int b) const noexcept {
		if (a == 0 || b == 0)
			return 0;

#ifdef ZX_REED_SOLOMON_USE_MORE_MEMORY_FOR_SPEED
		return _expTable[_logTable[a] + _logTable[b]];
#else
		return _expTable[fast_mod(_logTable[a] + _logTable[b], _size - 1)];
#endif
	}

	int size() const noexcept {
		return _size;
	}

	int generatorBase() const noexcept {
		return _generatorBase;
	}
};

} // namespace ZXing

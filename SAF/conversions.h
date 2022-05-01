/***** BEGIN LICENSE BLOCK *****
BSD License
Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***** END LICENCE BLOCK *****/

//Most of this is from nifskope
//https://github.com/niftools/nifskope/blob/develop/src/data/niftypes.h

#pragma once

#include "f4se/NiTypes.h"

enum {
	kRotationX = 1,
	kRotationY,
	kRotationZ
};

namespace SAF {

	constexpr double DEGREE_TO_RADIAN = MATH_PI / 180;
	constexpr double RADIAN_TO_DEGREE = 180 / MATH_PI;
	constexpr double HALF_PI = MATH_PI / 2;

	NiTransform TransformIdentity();

	void MatrixFromEulerYPR(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToEulerYPR(NiMatrix43& matrix, float& x, float& y, float& z);
	void MatrixFromEulerRPY(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToEulerRPY(NiMatrix43& matrix, float& x, float& y, float& z);
	void MatrixFromDegree(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToDegree(NiMatrix43 & matrix, float& x, float& y, float& z);

	NiTransform SlerpNiTransform(NiTransform& transform, float scalar);
	NiTransform NegateNiTransform(NiTransform& src, NiTransform& dest);
	void RotateMatrixXYZ(NiMatrix43& matrix, int type, float scalar);

	class Quat {
	public:
		float wxyz[4];

		Quat() {}

		Quat(float w, float x, float y, float z) {
			wxyz[0] = w;
			wxyz[1] = x;
			wxyz[2] = y;
			wxyz[3] = z;
		}

		void Normalize()
		{
			float mag = (
				(wxyz[0] * wxyz[0])
				+ (wxyz[1] * wxyz[1])
				+ (wxyz[2] * wxyz[2])
				+ (wxyz[3] * wxyz[3])
				);
			wxyz[0] /= mag;
			wxyz[1] /= mag;
			wxyz[2] /= mag;
			wxyz[3] /= mag;
		}

		void Negate()
		{
			wxyz[0] = -wxyz[0];
			wxyz[1] = -wxyz[1];
			wxyz[2] = -wxyz[2];
			wxyz[3] = -wxyz[3];
		}

		float& operator[](unsigned int i)
		{
			return wxyz[i];
		}

		const float& operator[](unsigned int i) const
		{
			return wxyz[i];
		}

		Quat operator*=(float s)
		{
			for (int c = 0; c < 4; c++)
				wxyz[c] *= s;

			return *this;
		}
		
		Quat operator*(float s) const
		{
			Quat q(*this);
			return (q *= s);
		}
	};
}
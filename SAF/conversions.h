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
	kAxisX = 1,
	kAxisY,
	kAxisZ
};

namespace SAF {

	constexpr double DEGREE_TO_RADIAN = MATH_PI / 180;
	constexpr double RADIAN_TO_DEGREE = 180 / MATH_PI;
	constexpr double HALF_PI = MATH_PI / 2;
	constexpr double DOUBLE_PI = MATH_PI * 2;

	NiMatrix43 MatrixIdentity();
	NiTransform TransformIdentity();

	void MatrixFromEulerYPRTransposed(NiMatrix43& matrix, float x, float y, float z);
	void MatrixFromEulerYPR(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToEulerYPRTransposed(const NiMatrix43& matrix, float& x, float& y, float& z);
	void MatrixToEulerYPR(const NiMatrix43& matrix, float& x, float& y, float& z);
	void MatrixFromEulerRPY(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToEulerRPY(const NiMatrix43& matrix, float& x, float& y, float& z);
	void MatrixFromDegree(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToDegree(const NiMatrix43 & matrix, float& x, float& y, float& z);
	void MatrixFromPose(NiMatrix43& matrix, float x, float y, float z);
	void MatrixToPose(const NiMatrix43& matrix, float& x, float& y, float& z);
	Vector3 MatrixToOutfitStudioVector(const NiMatrix43& matrix);

	NiPoint3 YPRToRPY(const NiPoint3& rot);
	NiPoint3 RPYToYPR(const NiPoint3& rot);

	NiPoint3 RotateMatrix(const NiMatrix43& m, const NiPoint3& pt);
	NiMatrix43 MultiplyNiMatrix(const NiMatrix43& lhs, const NiMatrix43& rhs);

	NiTransform MultiplyNiTransform(const NiTransform& lhs, const NiTransform& rhs);
	NiTransform SlerpNiTransform(const NiTransform& transform, float scalar);
	NiTransform NegateNiTransform(const NiTransform& src, const NiTransform& dest);
	NiTransform NegateNiTransformTransposed(const NiTransform& src, const NiTransform& dest);
	NiTransform InvertNiTransform(const NiTransform& t);

	bool TransformIsDefault(const NiTransform& t);
	bool TransformEqual(const NiTransform& lhs, const NiTransform& rhs);

	NiMatrix43 GetXYZRotation(int type, float scalar);
	void RotateMatrixXYZ(NiMatrix43& matrix, int type, float scalar);
	void RotateMatrixXYZ2(NiMatrix43& matrix, int type, float scalar);
	void RotateMatrixAxis(NiMatrix43& matrix, int type, float scalar);
	NiPoint3 GetMatrixAxis(const NiMatrix43& matrix, int type);

	class Quat {
	public:
		float wxyz[4];

		Quat() : wxyz{1.0f, 0.0f, 0.0f, 0.0f} {}

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

		Quat operator*(const Quat& q) const
		{
			Quat result;

			float a = wxyz[2] * q.wxyz[3] - wxyz[3] * q.wxyz[2];
			float b = wxyz[3] * q.wxyz[1] - wxyz[1] * q.wxyz[3];
			float c = wxyz[1] * q.wxyz[2] - wxyz[2] * q.wxyz[1];
			float d = wxyz[1] * q.wxyz[1] + wxyz[2] * q.wxyz[2] + wxyz[3] * q.wxyz[3];

			result.wxyz[1] = (wxyz[1] * q.wxyz[0] + q.wxyz[1] * wxyz[0]) + a;
			result.wxyz[2] = (wxyz[2] * q.wxyz[0] + q.wxyz[2] * wxyz[0]) + b;
			result.wxyz[3] = (wxyz[3] * q.wxyz[0] + q.wxyz[3] * wxyz[0]) + c;
			result.wxyz[0] = wxyz[0] * q.wxyz[0] - d;

			return result;
		}
	};

	Quat QuaternionIdentity();
}
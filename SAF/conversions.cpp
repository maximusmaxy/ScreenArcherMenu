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
//https://github.com/niftools/nifskope/blob/develop/src/data/niftypes.cpp

#include "conversions.h"

#include "f4se/NiObjects.h"

namespace SAF {

	float niMatrix43Identity[12]{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	float niTransformIdentity[16]{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	float quatIdentity[4]{ 1.0f, 0.0f, 0.0f, 0.0f };

	NiMatrix43 MatrixIdentity() {
		NiMatrix43 m;
		memcpy(m.arr, niMatrix43Identity, sizeof(niMatrix43Identity));
		return m;
	}

	NiTransform TransformIdentity() {
		NiTransform t;
		memcpy(&t, niTransformIdentity, sizeof(niTransformIdentity));
		return t;
	}

	Quat QuaternionIdentity() {
		Quat q;
		memcpy(&q, quatIdentity, sizeof(quatIdentity));
		return q;
	}

	NiPoint3 RotateMatrix(NiMatrix43& m, NiPoint3& pt) {
		return NiPoint3(
			m.data[0][0] * pt.x + m.data[1][0] * pt.y + m.data[2][0] * pt.z,
			m.data[0][1] * pt.x + m.data[1][1] * pt.y + m.data[2][1] * pt.z,
			m.data[0][2] * pt.x + m.data[1][2] * pt.y + m.data[2][2] * pt.z
		);
	}

	NiMatrix43 MultiplyNiMatrix(NiMatrix43& lhs, NiMatrix43& rhs)
	{
		NiMatrix43 tmp;
		tmp.data[0][0] =
			lhs.data[0][0] * rhs.data[0][0] +
			lhs.data[1][0] * rhs.data[0][1] +
			lhs.data[2][0] * rhs.data[0][2];
		tmp.data[0][1] =
			lhs.data[0][1] * rhs.data[0][0] +
			lhs.data[1][1] * rhs.data[0][1] +
			lhs.data[2][1] * rhs.data[0][2];
		tmp.data[0][2] =
			lhs.data[0][2] * rhs.data[0][0] +
			lhs.data[1][2] * rhs.data[0][1] +
			lhs.data[2][2] * rhs.data[0][2];
		tmp.data[1][0] =
			lhs.data[0][0] * rhs.data[1][0] +
			lhs.data[1][0] * rhs.data[1][1] +
			lhs.data[2][0] * rhs.data[1][2];
		tmp.data[1][1] =
			lhs.data[0][1] * rhs.data[1][0] +
			lhs.data[1][1] * rhs.data[1][1] +
			lhs.data[2][1] * rhs.data[1][2];
		tmp.data[1][2] =
			lhs.data[0][2] * rhs.data[1][0] +
			lhs.data[1][2] * rhs.data[1][1] +
			lhs.data[2][2] * rhs.data[1][2];
		tmp.data[2][0] =
			lhs.data[0][0] * rhs.data[2][0] +
			lhs.data[1][0] * rhs.data[2][1] +
			lhs.data[2][0] * rhs.data[2][2];
		tmp.data[2][1] =
			lhs.data[0][1] * rhs.data[2][0] +
			lhs.data[1][1] * rhs.data[2][1] +
			lhs.data[2][1] * rhs.data[2][2];
		tmp.data[2][2] =
			lhs.data[0][2] * rhs.data[2][0] +
			lhs.data[1][2] * rhs.data[2][1] +
			lhs.data[2][2] * rhs.data[2][2];
		return tmp;
	}

	NiTransform MultiplyNiTransform(NiTransform& lhs, NiTransform& rhs) {
		NiTransform tmp;
		tmp.scale = lhs.scale * rhs.scale;
		tmp.rot = MultiplyNiMatrix(lhs.rot, rhs.rot);
		tmp.pos = lhs.pos + RotateMatrix(lhs.rot, rhs.pos) * lhs.scale;
		return tmp;
	}

	NiTransform NegateNiTransform(NiTransform& src, NiTransform& dst) {
		NiTransform res;

		float srcScale = src.scale == 0 ? 1.0f : src.scale;
		res.scale = dst.scale / srcScale;

		NiMatrix43 inverted = src.rot.Transpose();
		res.rot = inverted * dst.rot;

		res.pos = inverted * ((dst.pos - src.pos) / srcScale);

		return res;
	}

	NiPoint3 Rotate(NiMatrix43& m, NiPoint3& pt) {
		return NiPoint3(
			m.data[0][0] * pt.x + m.data[1][0] * pt.y + m.data[2][0] * pt.z,
			m.data[0][1] * pt.x + m.data[1][1] * pt.y + m.data[2][1] * pt.z,
			m.data[0][2] * pt.x + m.data[1][2] * pt.y + m.data[2][2] * pt.z
		);
	}

	NiTransform NegateNiTransform2(NiTransform& src, NiTransform& dst) {
		NiTransform res;

		float srcScale = src.scale == 0 ? 1.0f : src.scale;
		res.scale = dst.scale / srcScale;

		NiMatrix43 inverted = src.rot.Transpose();
		res.rot = MultiplyNiMatrix(inverted, dst.rot);

		res.pos = RotateMatrix(inverted, (dst.pos - src.pos) / srcScale);

		return res;
	}

	NiMatrix43 GetXYZRotation(int type, float scalar) {
		NiMatrix43 rot;

		float sin = std::sinf(scalar);
		float cos = std::cosf(scalar);

		switch (type) {
		case kRotationX:
			rot.data[0][0] = 1.0f;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = cos;
			rot.data[1][2] = sin;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = -sin;
			rot.data[2][2] = cos;
			break;
		case kRotationY:
			rot.data[0][0] = cos;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = -sin;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = 1.0f;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = sin;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = cos;
			break;
		case kRotationZ:
			rot.data[0][0] = cos;
			rot.data[0][1] = sin;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = -sin;
			rot.data[1][1] = cos;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = 1.0f;
		}

		return rot;
	}

	void RotateMatrixXYZ(NiMatrix43& matrix, int type, float scalar) {
		NiMatrix43 rot;

		float sin = std::sinf(scalar);
		float cos = std::cosf(scalar);

		switch (type) {
		case kRotationX:
			rot.data[0][0] = 1.0f;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = cos;
			rot.data[1][2] = sin;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = -sin;
			rot.data[2][2] = cos;
			break;
		case kRotationY:
			rot.data[0][0] = cos;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = -sin;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = 1.0f;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = sin;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = cos;
			break;
		case kRotationZ:
			rot.data[0][0] = cos;
			rot.data[0][1] = sin;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = -sin;
			rot.data[1][1] = cos;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = 1.0f;
			break;
		}

		matrix = rot * matrix;
	}

	void RotateMatrixXYZ2(NiMatrix43& matrix, int type, float scalar) {
		NiMatrix43 rot;

		float sin = std::sinf(scalar);
		float cos = std::cosf(scalar);

		//switch (type) {
		//case kRotationX:
		//	rot.data[0][0] = 1.0f;
		//	rot.data[1][0] = 0.0f;
		//	rot.data[2][0] = 0.0f;
		//	rot.data[0][1] = 0.0f;
		//	rot.data[1][1] = cos;
		//	rot.data[2][1] = sin;
		//	rot.data[0][2] = 0.0f;
		//	rot.data[1][2] = -sin;
		//	rot.data[2][2] = cos;
		//	break;
		//case kRotationY:
		//	rot.data[0][0] = cos;
		//	rot.data[1][0] = 0.0f;
		//	rot.data[2][0] = -sin;
		//	rot.data[0][1] = 0.0f;
		//	rot.data[1][1] = 1.0f;
		//	rot.data[2][1] = 0.0f;
		//	rot.data[0][2] = sin;
		//	rot.data[1][2] = 0.0f;
		//	rot.data[2][2] = cos;
		//	break;
		//case kRotationZ:
		//	rot.data[0][0] = cos;
		//	rot.data[1][0] = sin;
		//	rot.data[2][0] = 0.0f;
		//	rot.data[0][1] = -sin;
		//	rot.data[1][1] = cos;
		//	rot.data[2][1] = 0.0f;
		//	rot.data[0][2] = 0.0f;
		//	rot.data[1][2] = 0.0f;
		//	rot.data[2][2] = 1.0f;
		//	break;
		//}

		switch (type) {
		case kRotationX:
			rot.data[0][0] = 1.0f;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = cos;
			rot.data[1][2] = sin;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = -sin;
			rot.data[2][2] = cos;
			break;
		case kRotationY:
			rot.data[0][0] = cos;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = -sin;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = 1.0f;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = sin;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = cos;
			break;
		case kRotationZ:
			rot.data[0][0] = cos;
			rot.data[0][1] = sin;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = -sin;
			rot.data[1][1] = cos;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = 1.0f;
			break;
		}

		matrix = MultiplyNiMatrix(rot, matrix);
	}

	void RotateMatrixAxis(NiMatrix43& matrix, int type, float scalar) {
		NiMatrix43 rot;

		float sin = std::sinf(scalar);
		float cos = std::cosf(scalar);

		switch (type) {
		case kRotationX:
			rot.data[0][0] = 1.0f;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = cos;
			rot.data[1][2] = sin;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = -sin;
			rot.data[2][2] = cos;
			break;
		case kRotationY:
			rot.data[0][0] = cos;
			rot.data[0][1] = 0.0f;
			rot.data[0][2] = -sin;
			rot.data[1][0] = 0.0f;
			rot.data[1][1] = 1.0f;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = sin;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = cos;
			break;
		case kRotationZ:
			rot.data[0][0] = cos;
			rot.data[0][1] = sin;
			rot.data[0][2] = 0.0f;
			rot.data[1][0] = -sin;
			rot.data[1][1] = cos;
			rot.data[1][2] = 0.0f;
			rot.data[2][0] = 0.0f;
			rot.data[2][1] = 0.0f;
			rot.data[2][2] = 1.0f;
			break;
		}

		matrix = matrix * rot;
	}

	void MatrixFromEulerYPR(NiMatrix43& matrix, float x, float y, float z) {
		float sinX = sin(x);
		float cosX = cos(x);
		float sinY = sin(y);
		float cosY = cos(y);
		float sinZ = sin(z);
		float cosZ = cos(z);

		matrix.data[0][0] = cosY * cosZ;
		matrix.data[0][1] = -cosY * sinZ;
		matrix.data[0][2] = sinY;
		matrix.data[1][0] = sinX * sinY * cosZ + sinZ * cosX;
		matrix.data[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		matrix.data[1][2] = -sinX * cosY;
		matrix.data[2][0] = sinX * sinZ - cosX * sinY * cosZ;
		matrix.data[2][1] = cosX * sinY * sinZ + sinX * cosZ;
		matrix.data[2][2] = cosX * cosY;
	}

	void MatrixFromEulerYPR2(NiMatrix43& matrix, float x, float y, float z) {
		float sinX = sin(x);
		float cosX = cos(x);
		float sinY = sin(y);
		float cosY = cos(y);
		float sinZ = sin(z);
		float cosZ = cos(z);

		matrix.data[0][0] = cosY * cosZ;
		matrix.data[1][0] = -cosY * sinZ;
		matrix.data[2][0] = sinY;
		matrix.data[0][1] = sinX * sinY * cosZ + sinZ * cosX;
		matrix.data[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		matrix.data[2][1] = -sinX * cosY;
		matrix.data[0][2] = sinX * sinZ - cosX * sinY * cosZ;
		matrix.data[1][2] = cosX * sinY * sinZ + sinX * cosZ;
		matrix.data[2][2] = cosX * cosY;
	}

	void MatrixToEulerYPR(NiMatrix43& matrix, float& x, float& y, float& z) {
		if (matrix.data[0][2] < 1.0) {
			if (matrix.data[0][2] > -1.0) {
				x = atan2(-matrix.data[1][2], matrix.data[2][2]);
				y = asin(matrix.data[0][2]);
				z = atan2(-matrix.data[0][1], matrix.data[0][0]);
			}
			else {
				x = -atan2(-matrix.data[1][0], matrix.data[1][1]);
				y = -HALF_PI;
				z = 0.0;
			}
		}
		else {
			x = atan2(matrix.data[1][0], matrix.data[1][1]);
			y = HALF_PI;
			z = 0.0;
		}
	}

	void MatrixToEulerYPR2(NiMatrix43& matrix, float& x, float& y, float& z) {
		if (matrix.data[2][0] < 1.0) {
			if (matrix.data[2][0] > -1.0) {
				x = atan2(-matrix.data[2][1], matrix.data[2][2]);
				y = asin(matrix.data[2][0]);
				z = atan2(-matrix.data[1][0], matrix.data[0][0]);
			}
			else {
				x = -atan2(-matrix.data[0][1], matrix.data[1][1]);
				y = -HALF_PI;
				z = 0.0;
			}
		}
		else {
			x = atan2(matrix.data[0][1], matrix.data[1][1]);
			y = HALF_PI;
			z = 0.0;
		}
	}

	void MatrixFromEulerRPY(NiMatrix43& matrix, float x, float y, float z) {
		float sinX = sin(x);
		float cosX = cos(x);
		float sinY = sin(y);
		float cosY = cos(y);
		float sinZ = sin(z);
		float cosZ = cos(z);

		matrix.data[0][0] = cosX * cosY;
		matrix.data[0][1] = cosX * sinY * sinZ - cosZ * sinX;
		matrix.data[0][2] = sinX * sinZ + cosX * cosZ * sinY;
		matrix.data[1][0] = cosY * sinX;
		matrix.data[1][1] = cosX * cosZ + sinX * sinY * sinZ;
		matrix.data[1][2] = cosZ * sinX * sinY - cosX * sinZ;
		matrix.data[2][0] = -sinY;
		matrix.data[2][1] = cosY * sinZ;
		matrix.data[2][2] = cosY * cosZ;
	}

	void MatrixToEulerRPY(NiMatrix43& matrix, float& x, float& y, float& z) {
		if (matrix.data[2][0] < 1.0) {
			if (matrix.data[2][0] > -1.0) {
				x = atan2(matrix.data[1][0], matrix.data[0][0]);
				y = asin(-matrix.data[2][0]);
				z = atan2(matrix.data[2][1], matrix.data[2][2]);
			}
			else {
				x = -atan2(matrix.data[0][1], matrix.data[1][1]);
				y = HALF_PI;
				z = 0.0f;
			}
		}
		else {
			x = atan2(-matrix.data[0][1], matrix.data[1][1]);
			y = -HALF_PI;
			z = 0.0f;
		}
	}

	void MatrixFromDegree(NiMatrix43& matrix, float x, float y, float z) {
		MatrixFromEulerYPR(matrix, x * -DEGREE_TO_RADIAN, y * -DEGREE_TO_RADIAN, z * -DEGREE_TO_RADIAN);
	}

	void MatrixToDegree(NiMatrix43& matrix, float& x, float& y, float& z) {
		MatrixToEulerYPR(matrix, x, y, z);
		x *= -RADIAN_TO_DEGREE;
		y *= -RADIAN_TO_DEGREE;
		z *= -RADIAN_TO_DEGREE;
	}

	void MatrixFromPose(NiMatrix43& matrix, float x, float y, float z) {
		MatrixFromEulerYPR2(matrix, x * DEGREE_TO_RADIAN, y * DEGREE_TO_RADIAN, z * DEGREE_TO_RADIAN);
	}

	void MatrixToPose(NiMatrix43& matrix, float& x, float& y, float& z) {
		MatrixToEulerYPR2(matrix, x, y, z);
		x *= RADIAN_TO_DEGREE;
		y *= RADIAN_TO_DEGREE;
		z *= RADIAN_TO_DEGREE;
	}

	NiPoint3 YPRToRPY(NiPoint3& rot)
	{
		NiMatrix43 matrix;
		MatrixFromEulerYPR(matrix, -rot.x, -rot.y, -rot.z);

		NiPoint3 newRot;
		MatrixToEulerRPY(matrix, newRot.x, newRot.y, newRot.z);

		return newRot;
	}

	NiPoint3 RPYToYPR(NiPoint3& rot)
	{
		NiMatrix43 matrix;
		MatrixFromEulerRPY(matrix, rot.x, rot.y, rot.z);

		NiPoint3 newRot;
		MatrixToEulerYPR(matrix, newRot.x, newRot.y, newRot.z);
		newRot.x *= -1;
		newRot.y *= -1;
		newRot.z *= -1;

		return newRot;
	}

	NiMatrix43 NiMatrix43Invert(NiMatrix43& matrix) {
		NiMatrix43 i;

		i.data[0][0] = matrix.data[1][1] * matrix.data[2][2] - matrix.data[1][2] * matrix.data[2][1];
		i.data[0][1] = matrix.data[0][2] * matrix.data[2][1] - matrix.data[0][1] * matrix.data[2][2];
		i.data[0][2] = matrix.data[0][1] * matrix.data[1][2] - matrix.data[0][2] * matrix.data[1][1];
		i.data[1][0] = matrix.data[1][2] * matrix.data[2][0] - matrix.data[1][0] * matrix.data[2][2];
		i.data[1][1] = matrix.data[0][0] * matrix.data[2][2] - matrix.data[0][2] * matrix.data[2][0];
		i.data[1][2] = matrix.data[0][2] * matrix.data[1][0] - matrix.data[0][0] * matrix.data[1][2];
		i.data[2][0] = matrix.data[1][0] * matrix.data[2][1] - matrix.data[1][1] * matrix.data[2][0];
		i.data[2][1] = matrix.data[0][1] * matrix.data[2][0] - matrix.data[0][0] * matrix.data[2][1];
		i.data[2][2] = matrix.data[0][0] * matrix.data[1][1] - matrix.data[0][1] * matrix.data[1][0];

		float d = matrix.data[0][0] * i.data[0][0] + matrix.data[0][1] * i.data[1][0] + matrix.data[0][2] * i.data[2][0];

		if (fabs(d) <= 0.0)
			return MatrixIdentity();

		for (int x = 0; x < 3; x++) {
			for (int y = 0; y < 3; y++)
				i.data[x][y] /= d;
		}

		return i;
	}

	NiMatrix43 NiFromQuat(Quat& q)
	{
		float fTx = ((float)2.0) * q[1];
		float fTy = ((float)2.0) * q[2];
		float fTz = ((float)2.0) * q[3];
		float fTwx = fTx * q[0];
		float fTwy = fTy * q[0];
		float fTwz = fTz * q[0];
		float fTxx = fTx * q[1];
		float fTxy = fTy * q[1];
		float fTxz = fTz * q[1];
		float fTyy = fTy * q[2];
		float fTyz = fTz * q[2];
		float fTzz = fTz * q[3];

		NiMatrix43 m;

		m.data[0][0] = (float)1.0 - (fTyy + fTzz);
		m.data[0][1] = fTxy - fTwz;
		m.data[0][2] = fTxz + fTwy;
		m.data[1][0] = fTxy + fTwz;
		m.data[1][1] = (float)1.0 - (fTxx + fTzz);
		m.data[1][2] = fTyz - fTwx;
		m.data[2][0] = fTxz - fTwy;
		m.data[2][1] = fTyz + fTwx;
		m.data[2][2] = (float)1.0 - (fTxx + fTyy);

		return m;
	}

	Quat NiToQuat(NiMatrix43& m)
	{
		Quat q;

		float trace = m.data[0][0] + m.data[1][1] + m.data[2][2];
		float root;

		if (trace > 0.0) {
			root = sqrt(trace + 1.0);
			q[0] = root / 2.0;
			root = 0.5 / root;
			q[1] = (m.data[2][1] - m.data[1][2]) * root;
			q[2] = (m.data[0][2] - m.data[2][0]) * root;
			q[3] = (m.data[1][0] - m.data[0][1]) * root;
		}
		else {
			int i = (m.data[1][1] > m.data[0][0] ? 1 : 0);

			if (m.data[2][2] > m.data[i][i])
				i = 2;

			const int next[3] = {
				1, 2, 0
			};
			int j = next[i];
			int k = next[j];

			root = sqrt(m.data[i][i] - m.data[j][j] - m.data[k][k] + 1.0);
			q[i + 1] = root / 2;
			root = 0.5 / root;
			q[0] = (m.data[k][j] - m.data[j][k]) * root;
			q[j + 1] = (m.data[j][i] + m.data[i][j]) * root;
			q[k + 1] = (m.data[k][i] + m.data[i][k]) * root;
		}

		return q;
	}

	static inline float ISqrt_approx_in_neighborhood(float s)
	{
		static const float NEIGHBORHOOD = 0.959066f;
		static const float SCALE = 1.000311f;
		static const float ADDITIVE_CONSTANT = SCALE / (float)sqrt(NEIGHBORHOOD);
		static const float FACTOR = SCALE * (-0.5f / (NEIGHBORHOOD * (float)sqrt(NEIGHBORHOOD)));
		return ADDITIVE_CONSTANT + (s - NEIGHBORHOOD) * FACTOR;
	}

	static inline void fast_normalize(Quat& q)
	{
		float s = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
		float k = ISqrt_approx_in_neighborhood(s);

		if (s <= 0.91521198f) {
			k *= ISqrt_approx_in_neighborhood(k * k * s);

			if (s <= 0.65211970f) {
				k *= ISqrt_approx_in_neighborhood(k * k * s);
			}
		}

		q[0] *= k; q[1] *= k; q[2] *= k; q[3] *= k;
	}

	static inline float lerp(float v0, float v1, float perc)
	{
		return v0 + perc * (v1 - v0);
	}

	static inline float correction(float t, float fCos)
	{
		const float s = 0.8228677f;
		const float kc = 0.5855064f;
		float factor = 1.0f - s * fCos;
		float k = kc * factor * factor;
		return t * (k * t * (2.0f * t - 3.0f) + 1.0f + k);
	}

	static float dotproduct(const Quat& q1, const Quat& q2)
	{
		return q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
	}

	Quat SlerpQuat(const Quat& p, const Quat& q, float t)
	{
		// Copyright (c) 2002 Jonathan Blow
		//  "Hacking Quaternions", The Inner Product, March 2002
		//   http://number-none.com/product/Hacking%20Quaternions/index.html

		float tprime;

		if (t <= 0.5f) {
			tprime = correction(t, dotproduct(p, q));
		}
		else {
			tprime = 1.0f - correction(1.0f - t, dotproduct(p, q));
		}

		Quat result(lerp(p[0], q[0], tprime), lerp(p[1], q[1], tprime),
			lerp(p[2], q[2], tprime), lerp(p[3], q[3], tprime));
		fast_normalize(result);
		return result;
	}

	NiPoint3 NormalizeVector(NiPoint3 &v)
	{
		NiPoint3 result;

		float m = std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

		if (m > 0.0)
			m = 1.0 / m;
		else
			m = 0.0F;

		result.x = v.x * m;
		result.y = v.y * m;
		result.z = v.z * m;

		return result;
	}

	Quat QuatFromAxisAngle(NiPoint3 axis, float angle)
	{
		Quat result;
		//NormalizeVector(axis); already normalized

		float s = std::sinf(angle / 2);
		result.wxyz[0] = std::cosf(angle / 2);
		result.wxyz[1] = s * axis.x;
		result.wxyz[2] = s * axis.y;
		result.wxyz[3] = s * axis.z;

		return result;
	}

	NiTransform SlerpNiTransform(NiTransform& transform, float scalar) {
		NiTransform res;

		res.pos = transform.pos * scalar;
		res.scale = 1.0f + ((transform.scale - 1.0f) * scalar);

		static Quat identity = QuaternionIdentity();

		Quat niQuat = NiToQuat(transform.rot);
		Quat slerpedQuat = SlerpQuat(identity, niQuat, scalar);

		res.rot = NiFromQuat(slerpedQuat);

		return res;
	}

	//Conversion to outfit studio rotation vector https://github.com/ousnius/nifly/blob/main/src/Object3d.cpp#L46
	Vector3 MatrixToOutfitStudioVector(NiMatrix43& m)
	{
		double cosang = (m.data[0][0] + m.data[1][1] + m.data[2][2] - 1) * 0.5;

		if (cosang > 0.5) {
			Vector3 v(m.data[1][2] - m.data[2][1], m.data[2][0] - m.data[0][2], m.data[0][1] - m.data[1][0]);
			double sin2ang = v.Magnitude();
			if (sin2ang == 0.0)
				return Vector3();
			v.Scale(static_cast<float>(std::asin(sin2ang * 0.5) / sin2ang));
			return v;
		}

		if (cosang > -1) {
			Vector3 v(m.data[1][2] - m.data[2][1], m.data[2][0] - m.data[0][2], m.data[0][1] - m.data[1][0]);
			v.Normalize();
			v.Scale(static_cast<float>(std::acos(cosang)));
			return v;
		}

		// cosang <= -1, sinang == 0
		double x = (m.data[0][0] - cosang) * 0.5;
		double y = (m.data[1][1] - cosang) * 0.5;
		double z = (m.data[2][2] - cosang) * 0.5;

		// Solve precision issues that would cause NaN
		if (x < 0.0)
			x = 0.0;
		if (y < 0.0)
			y = 0.0;
		if (z < 0.0)
			z = 0.0;

		Vector3 v(static_cast<float>(std::sqrt(x)),
			static_cast<float>(std::sqrt(y)),
			static_cast<float>(std::sqrt(z)));

		v.Normalize();

		if (m.data[1][2] < m.data[2][1])
			v.x = -v.x;
		if (m.data[2][0] < m.data[0][2])
			v.y = -v.y;
		if (m.data[0][1] < m.data[1][0])
			v.z = -v.z;

		v.Scale(MATH_PI);

		return v;
	}

	//Conversion from outfit studio rotation vector https://github.com/ousnius/nifly/blob/main/src/Object3d.cpp#L21
	//NiMatrix43 MatrixFromOutfitStudioVector(NiPoint3& pt)
	//{
	//	double angle = std::sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
	//	double cosang = std::cos(angle);
	//	double sinang = std::sin(angle);
	//	double onemcosang = NAN; // One minus cosang
	//	// Avoid loss of precision from cancellation in calculating onemcosang
	//	if (cosang > .5)
	//		onemcosang = sinang * sinang / (1 + cosang);
	//	else
	//		onemcosang = 1 - cosang;

	//	Vector3 n = angle != 0.0 ? v / static_cast<float>(angle) : Vector3(1.0f, 0.0f, 0.0f);
	//	Matrix3 m;
	//	m[0][0] = n.x * n.x * static_cast<float>(onemcosang) + static_cast<float>(cosang);
	//	m[1][1] = n.y * n.y * static_cast<float>(onemcosang) + static_cast<float>(cosang);
	//	m[2][2] = n.z * n.z * static_cast<float>(onemcosang) + static_cast<float>(cosang);
	//	m[0][1] = n.x * n.y * static_cast<float>(onemcosang) + n.z * static_cast<float>(sinang);
	//	m[1][0] = n.x * n.y * static_cast<float>(onemcosang) - n.z * static_cast<float>(sinang);
	//	m[1][2] = n.y * n.z * static_cast<float>(onemcosang) + n.x * static_cast<float>(sinang);
	//	m[2][1] = n.y * n.z * static_cast<float>(onemcosang) - n.x * static_cast<float>(sinang);
	//	m[2][0] = n.z * n.x * static_cast<float>(onemcosang) + n.y * static_cast<float>(sinang);
	//	m[0][2] = n.z * n.x * static_cast<float>(onemcosang) - n.y * static_cast<float>(sinang);
	//	return m;
	//}
}
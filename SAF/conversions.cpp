#include "conversions.h"

#include "f4se/NiObjects.h"

#include <vector>

using namespace Eigen;

namespace SAF {
	Matrix3f NiToEigen(NiMatrix43 ni) {
		return Matrix3f{
			{ ni.arr[0], ni.arr[1], ni.arr[2] },
			{ ni.arr[4], ni.arr[5], ni.arr[6] },
			{ ni.arr[8], ni.arr[9], ni.arr[10]}
		};
	}

	NiMatrix43 NiFromEigen(Matrix3f eigen) {
		NiMatrix43 res;
		res.arr[0] = eigen(0, 0);
		res.arr[1] = eigen(0, 1);
		res.arr[2] = eigen(0, 2);
		res.arr[4] = eigen(1, 0);
		res.arr[5] = eigen(1, 1);
		res.arr[6] = eigen(1, 2);
		res.arr[8] = eigen(2, 0);
		res.arr[9] = eigen(2, 1);
		res.arr[10] = eigen(2, 2);
		return res;
	}

	NiTransform SafTransform::ToNi() {
		NiTransform transform;
		transform.pos = NiPoint3(x, y, z);
		transform.rot.SetEulerAngles(yaw, pitch, roll);
		transform.scale = scale;
		return transform;
	}

	void SafTransform::FromNi(NiTransform transform) {

	}

	Matrix3f SafTransform::GetMatrix() {
		return Matrix3f();
	}

	void SafTransform::SetMatrix(Matrix3f matrix) {

	}

	NiTransform SafTransform::Slerp(float scalar) {
		NiTransform res;

		res.pos = NiPoint3(x * scalar, y * scalar, z * scalar);

		res.scale = 1.0f + ((scale - 1.0f) * scalar);

		Quaternionf quat(GetMatrix());
		Quaternionf rotationRes = Quaternionf::Identity().slerp(scalar, quat);

		res.rot = NiFromEigen(rotationRes.matrix());

		return res;
	}

	//https://en.wikipedia.org/wiki/Euler_angles#Conversion_to_other_orientation_representations
	//ZYX Tailt Bryan angles with (0, 1), (1, 0), (1, 2), (2, 1) inverted

	void GetEulerAngles(Matrix3f matrix, float* yaw, float* pitch, float* roll)
	{
		*yaw = atan2(-matrix(1, 2), matrix(2, 2));
		*pitch = atan2(-matrix(0, 2), std::sqrt(1 - matrix(0, 2) * matrix(0, 2)));
		*roll = atan2(-matrix(0, 1), matrix(0, 0));
	}

	void SetEulerAngles(Matrix3f* matrix, float yaw, float pitch, float roll)
	{
		float ySin = sin(yaw);
		float yCos = cos(yaw);
		float pSin = sin(pitch);
		float pCos = cos(pitch);
		float rSin = sin(roll);
		float rCos = cos(roll);

		(*matrix)(0, 0) = pCos * rCos;
		(*matrix)(0, 1) = -ySin * pSin * rCos + yCos * rSin;
		(*matrix)(0, 2) = yCos * pSin * rCos + ySin * rSin;
		(*matrix)(1, 0) = -pCos * rSin;
		(*matrix)(1, 1) = ySin * pSin * rSin + yCos * rCos;
		(*matrix)(1, 2) = -yCos * pSin * rSin + ySin * rCos;
		(*matrix)(2, 0) = -pSin;
		(*matrix)(2, 1) = -ySin * pCos;
		(*matrix)(2, 2) = yCos * pCos;
	}

	NiTransform SlerpNiTransform(NiTransform transform, float scalar) {
		NiTransform res;

		res.pos = transform.pos * scalar;
		res.scale = 1.0f + ((transform.scale - 1.0f) * scalar);

		Matrix3f matrix = NiToEigen(transform.rot);
		Quaternionf quat(matrix);
		Quaternionf rotationRes = Quaternionf::Identity().slerp(scalar, quat);

		res.rot = NiFromEigen(rotationRes.matrix());

		return res;
	}

	NiTransform NegateNiTransform(NiTransform src, NiTransform dst) {
		NiTransform res;

		res.pos = dst.pos - src.pos;
		if (src.scale == 0)
			res.scale = 1.0f;
		else
			res.scale = dst.scale / src.scale;

		NiMatrix43 dif = dst.rot * src.rot.Transpose();
		res.rot = dif.Transpose() * dst.rot;

		//float h, a, b;
		//res.rot.GetEulerAngles(&h, &a, &b);
		//h *= -1;
		//res.rot.SetEulerAngles(h, a, b);

		return res;
	}

	NiTransform NegateNiTransform2(NiTransform src, NiTransform dst) {
		NiTransform res;

		res.pos = dst.pos - src.pos;
		if (src.scale == 0)
			res.scale = 1.0f;
		else
			res.scale = dst.scale / src.scale;

		NiMatrix43 dif = src.rot * dst.rot.Transpose();
		res.rot = dif.Transpose() * dst.rot;

		//float h, a, b;
		//res.rot.GetEulerAngles(&h, &a, &b);
		//h *= -1;
		//res.rot.SetEulerAngles(h, a, b);

		return res;
	}

	//NiTransform NegateNiTransform2(NiTransform src, NiTransform dst) {
	//	NiTransform res;
	//
	//	res.pos = dst.pos - src.pos;
	//	if (src.scale == 0)
	//		res.scale = 1.0f;
	//	else
	//		res.scale = dst.scale / src.scale;
	//
	//	NiMatrix43 dif = dst.rot * src.rot.Transpose();
	//	res.rot = dst.rot * dif;
	//
	//	return res;
	//}

	//NiPoint3 GetForwardKinematicPos(std::vector<NiAVObject*> nodes)
	//{
	//
	//}
	//
	//void ApplyInverseKinematics(std::vector<NiAVObject*> nodes, NiPoint3 dest)
	//{
	//	NiTransform front = nodes.front()->m_worldTransform;
	//	NiTransform back = nodes.back()->m_worldTransform;
	//	double error = std::abs(dest.x - back.pos.x) + std::abs(dest.y - back.pos.y) + std::abs(dest.z - back.pos.z);
	//
	//	std::vector<Matrix3f> rotations;
	//	for (auto& node : nodes) {
	//		rotations.push_back(NiToEigen(node->m_localTransform.rot));
	//	}
	//	UInt32 index = rotations.size() - 1;
	//
	//	while (error < 0.01) {
	//		
	//	}
	//}
}
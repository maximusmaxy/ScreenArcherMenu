#pragma once

#include "f4se/NiTypes.h"

#include <Eigen/Dense>

namespace SAF {
	class SafTransform {
	public:
		float x;
		float y;
		float z;
		float yaw;
		float pitch;
		float roll;
		float scale;

		SafTransform() :
			x(0.0f), y(0.0f), z(0.0f),
			yaw(0.0f), pitch(0.0f), roll(0.0f),
			scale(1.0f)
		{}

		SafTransform(float x, float y, float z, float yaw, float pitch, float roll, float scale) :
			x(x), y(y), z(z),
			yaw(yaw), pitch(pitch), roll(roll),
			scale(scale)
		{}

		SafTransform(NiTransform transform)
		{
			x = transform.pos.x;
			y = transform.pos.y;
			z = transform.pos.z;
			transform.rot.GetEulerAngles(&yaw, &pitch, &roll);
			scale = transform.scale;
		}

		NiTransform ToNi();
		void FromNi(NiTransform transform);

		Eigen::Matrix3f GetMatrix();
		void SetMatrix(Eigen::Matrix3f);

		NiTransform Slerp(float scalar);
	};


	NiTransform SlerpNiTransform(NiTransform transform, float scalar);
	NiTransform NegateNiTransform(NiTransform src, NiTransform dest);
	NiTransform NegateNiTransform2(NiTransform src, NiTransform dest);
}
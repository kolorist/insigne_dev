#pragma once

#include <floral.h>

struct Camera {
	floral::vec3f								Position;
	floral::vec3f								LookAtDir;
	f32											AspectRatio;
	f32											NearPlane;
	f32											FarPlane;
	f32											FOV;

	floral::mat4x4f								ViewMatrix;
	floral::mat4x4f								ProjectionMatrix;
	floral::mat4x4f								WVPMatrix;
};

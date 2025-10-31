#include "Camera.h"
#include "Input.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void MatIdentity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MatPerspective(float m[16], float fovy, float aspect, float zn, float zf) {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.f;
    m[14] = (2.f * zf * zn) / (zn - zf);
}

static void MatLookAt(float m[16],
    float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ) {
    // Forward = normalize(center - eye)
    float fX = centerX - eyeX;
    float fY = centerY - eyeY;
    float fZ = centerZ - eyeZ;
    float len = std::sqrt(fX * fX + fY * fY + fZ * fZ);
    if (len > 0.00001f) { fX /= len; fY /= len; fZ /= len; }

    // Right = normalize(cross(forward, up))
    float rX = fY * upZ - fZ * upY;
    float rY = fZ * upX - fX * upZ;
    float rZ = fX * upY - fY * upX;
    len = std::sqrt(rX * rX + rY * rY + rZ * rZ);
    if (len > 0.00001f) { rX /= len; rY /= len; rZ /= len; }

    // Up = cross(right, forward)
    float uX = rY * fZ - rZ * fY;
    float uY = rZ * fX - rX * fZ;
    float uZ = rX * fY - rY * fX;

    MatIdentity(m);
    m[0] = rX;  m[4] = rY;  m[8] = rZ;  m[12] = -(rX * eyeX + rY * eyeY + rZ * eyeZ);
    m[1] = uX;  m[5] = uY;  m[9] = uZ;  m[13] = -(uX * eyeX + uY * eyeY + uZ * eyeZ);
    m[2] = -fX; m[6] = -fY; m[10] = -fZ; m[14] = (fX * eyeX + fY * eyeY + fZ * eyeZ);
}

Camera::Camera()
    : mPosX(0), mPosY(0), mPosZ(5)
    , mYaw(-90.0f)
    , mPitch(0.0f)
    , mSpeed(5.0f)
    , mSensitivity(0.1f)
    , mFOV(45.0f)
{
    UpdateVectors();
}

void Camera::Update(float deltaTime) {
    // Solo procesar input si el modo cámara está activo
    if (!Input::IsCameraControlActive()) {
        return;
    }

    // Rotación con ratón
    int dx, dy;
    Input::GetMouseDelta(dx, dy);

    if (dx != 0 || dy != 0) {
        mYaw += dx * mSensitivity;
        mPitch -= dy * mSensitivity;

        // Limitar pitch para evitar gimbal lock
        mPitch = std::max(-89.0f, std::min(89.0f, mPitch));

        UpdateVectors();
    }

    // Movimiento con WASD
    float velocity = mSpeed * deltaTime;

    if (Input::IsKeyDown(SDLK_W)) {
        mPosX += mForwardX * velocity;
        mPosY += mForwardY * velocity;
        mPosZ += mForwardZ * velocity;
    }
    if (Input::IsKeyDown(SDLK_S)) {
        mPosX -= mForwardX * velocity;
        mPosY -= mForwardY * velocity;
        mPosZ -= mForwardZ * velocity;
    }
    if (Input::IsKeyDown(SDLK_A)) {
        mPosX -= mRightX * velocity;
        mPosY -= mRightY * velocity;
        mPosZ -= mRightZ * velocity;
    }
    if (Input::IsKeyDown(SDLK_D)) {
        mPosX += mRightX * velocity;
        mPosY += mRightY * velocity;
        mPosZ += mRightZ * velocity;
    }
    if (Input::IsKeyDown(SDLK_E)) {
        mPosY += velocity; // Subir
    }
    if (Input::IsKeyDown(SDLK_Q)) {
        mPosY -= velocity; // Bajar
    }

    // Zoom con rueda del ratón
    float wheel = Input::GetMouseWheelDelta();
    if (wheel != 0.0f) {
        Zoom(wheel);
    }
}

void Camera::UpdateVectors() {
    float yawRad = mYaw * (float)M_PI / 180.0f;
    float pitchRad = mPitch * (float)M_PI / 180.0f;

    // Forward
    mForwardX = std::cos(yawRad) * std::cos(pitchRad);
    mForwardY = std::sin(pitchRad);
    mForwardZ = std::sin(yawRad) * std::cos(pitchRad);

    float len = std::sqrt(mForwardX * mForwardX + mForwardY * mForwardY + mForwardZ * mForwardZ);
    if (len > 0.00001f) {
        mForwardX /= len;
        mForwardY /= len;
        mForwardZ /= len;
    }

    // Right = normalize(cross(forward, worldUp))
    float worldUpX = 0, worldUpY = 1, worldUpZ = 0;
    mRightX = mForwardY * worldUpZ - mForwardZ * worldUpY;
    mRightY = mForwardZ * worldUpX - mForwardX * worldUpZ;
    mRightZ = mForwardX * worldUpY - mForwardY * worldUpX;

    len = std::sqrt(mRightX * mRightX + mRightY * mRightY + mRightZ * mRightZ);
    if (len > 0.00001f) {
        mRightX /= len;
        mRightY /= len;
        mRightZ /= len;
    }

    // Up = cross(right, forward)
    mUpX = mRightY * mForwardZ - mRightZ * mForwardY;
    mUpY = mRightZ * mForwardX - mRightX * mForwardZ;
    mUpZ = mRightX * mForwardY - mRightY * mForwardX;
}

void Camera::GetViewMatrix(float out[16]) const {
    float centerX = mPosX + mForwardX;
    float centerY = mPosY + mForwardY;
    float centerZ = mPosZ + mForwardZ;

    MatLookAt(out, mPosX, mPosY, mPosZ, centerX, centerY, centerZ, mUpX, mUpY, mUpZ);
}

void Camera::GetProjectionMatrix(float out[16], float aspect) const {
    MatPerspective(out, mFOV * (float)M_PI / 180.0f, aspect, 0.1f, 1000.0f);
}

void Camera::SetPosition(float x, float y, float z) {
    mPosX = x;
    mPosY = y;
    mPosZ = z;
}

void Camera::Rotate(float yaw, float pitch) {
    mYaw += yaw;
    mPitch += pitch;
    mPitch = std::max(-89.0f, std::min(89.0f, mPitch));
    UpdateVectors();
}

void Camera::Zoom(float amount) {
    mFOV -= amount * 2.0f;
    mFOV = std::max(1.0f, std::min(90.0f, mFOV));
}

void Camera::GetPosition(float& x, float& y, float& z) const {
    x = mPosX;
    y = mPosY;
    z = mPosZ;
}
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
    std::cout << "Camera initialized" << std::endl;
}

void Camera::Update(float deltaTime) {
    if (!Input::IsCameraControlActive()) {
        return;
    }

    // Rotación con ratón
    int dx, dy;
    Input::GetMouseDelta(dx, dy);

    if (dx != 0 || dy != 0) {
        mYaw += dx * mSensitivity;
        mPitch -= dy * mSensitivity;

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
        mPosY += velocity;
    }
    if (Input::IsKeyDown(SDLK_Q)) {
        mPosY -= velocity;
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
    // CAMBIO: near plane de 0.01 en lugar de 0.1 para ver objetos muy cerca
    MatPerspective(out, mFOV * (float)M_PI / 180.0f, aspect, 0.01f, 1000.0f);
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
    mFOV = std::max(10.0f, std::min(90.0f, mFOV)); // Rango 10-90 en lugar de 1-90

    static int lastPrint = 0;
    if (++lastPrint % 10 == 0) {
        std::cout << "FOV: " << mFOV << " degrees" << std::endl;
    }
}

void Camera::GetPosition(float& x, float& y, float& z) const {
    x = mPosX;
    y = mPosY;
    z = mPosZ;
}

void Camera::FocusOnPoint(float targetX, float targetY, float targetZ, float distance) {
    std::cout << "\n=== FocusOnPoint ===" << std::endl;
    std::cout << "Target: (" << targetX << ", " << targetY << ", " << targetZ << ")" << std::endl;
    std::cout << "Distance: " << distance << std::endl;

    // Asegurar una distancia mínima razonable
    if (distance < 2.0f) distance = 2.0f;

    // Posicionar cámara en un ángulo isométrico agradable
    // 45 grados horizontal, 30 grados vertical
    float angleH = 45.0f * (float)M_PI / 180.0f;
    float angleV = 30.0f * (float)M_PI / 180.0f;

    // Calcular offset desde el target
    float offsetX = distance * std::cos(angleV) * std::cos(angleH);
    float offsetY = distance * std::sin(angleV);
    float offsetZ = distance * std::cos(angleV) * std::sin(angleH);

    // Posicionar cámara
    mPosX = targetX + offsetX;
    mPosY = targetY + offsetY;
    mPosZ = targetZ + offsetZ;

    std::cout << "Camera positioned at: (" << mPosX << ", " << mPosY << ", " << mPosZ << ")" << std::endl;

    // Hacer que la cámara mire exactamente al target
    LookAt(targetX, targetY, targetZ);

    std::cout << "Camera yaw: " << mYaw << ", pitch: " << mPitch << std::endl;
    std::cout << "Camera forward: (" << mForwardX << ", " << mForwardY << ", " << mForwardZ << ")" << std::endl;

    // Verificar la distancia real
    float dx = mPosX - targetX;
    float dy = mPosY - targetY;
    float dz = mPosZ - targetZ;
    float actualDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    std::cout << "Actual distance from target: " << actualDistance << std::endl;
}

void Camera::LookAt(float targetX, float targetY, float targetZ) {
    // Calcular dirección hacia el target
    float dirX = targetX - mPosX;
    float dirY = targetY - mPosY;
    float dirZ = targetZ - mPosZ;

    // Normalizar
    float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (len < 0.00001f) return;

    dirX /= len;
    dirY /= len;
    dirZ /= len;

    // Calcular yaw (rotación horizontal)
    // atan2(z, x) en el plano XZ
    mYaw = std::atan2(dirZ, dirX) * 180.0f / (float)M_PI;

    // Calcular pitch (rotación vertical)
    // asin(y) para el ángulo vertical
    mPitch = std::asin(dirY) * 180.0f / (float)M_PI;

    // Limitar pitch
    mPitch = std::max(-89.0f, std::min(89.0f, mPitch));

    UpdateVectors();
}
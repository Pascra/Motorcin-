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
    // 🔧 FIX: Asegurar que los parámetros son válidos
    if (aspect <= 0.0f) aspect = 1.0f;
    if (zn <= 0.0f) zn = 0.01f;
    if (zf <= zn) zf = zn + 1000.0f;

    const float f = 1.0f / std::tan(fovy * 0.5f);

    for (int i = 0; i < 16; ++i) m[i] = 0.f;

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.f;
    m[14] = (2.f * zf * zn) / (zn - zf);
    m[15] = 0.f;
}

static void MatLookAt(float m[16],
    float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ) {
    float fX = centerX - eyeX;
    float fY = centerY - eyeY;
    float fZ = centerZ - eyeZ;
    float len = std::sqrt(fX * fX + fY * fY + fZ * fZ);
    if (len > 0.00001f) { fX /= len; fY /= len; fZ /= len; }

    float rX = fY * upZ - fZ * upY;
    float rY = fZ * upX - fX * upZ;
    float rZ = fX * upY - fY * upX;
    len = std::sqrt(rX * rX + rY * rY + rZ * rZ);
    if (len > 0.00001f) { rX /= len; rY /= len; rZ /= len; }

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
    , mSceneSize(10.0f)
{
    UpdateVectors();
    std::cout << "[CAM] Initialized at (" << mPosX << ", " << mPosY << ", " << mPosZ << ")" << std::endl;
    std::cout << "[CAM] Yaw: " << mYaw << ", Pitch: " << mPitch << ", FOV: " << mFOV << std::endl;
}

void Camera::Update(float deltaTime) {
    if (!Input::IsCameraControlActive()) {
        return;
    }

    int dx, dy;
    Input::GetMouseDelta(dx, dy);

    if (dx != 0 || dy != 0) {
        mYaw += dx * mSensitivity;
        mPitch -= dy * mSensitivity;
        mPitch = std::max(-89.0f, std::min(89.0f, mPitch));
        UpdateVectors();
    }

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

    float wheel = Input::GetMouseWheelDelta();
    if (wheel != 0.0f) {
        Zoom(wheel);
    }
}

void Camera::UpdateVectors() {
    float yawRad = mYaw * (float)M_PI / 180.0f;
    float pitchRad = mPitch * (float)M_PI / 180.0f;

    mForwardX = std::cos(yawRad) * std::cos(pitchRad);
    mForwardY = std::sin(pitchRad);
    mForwardZ = std::sin(yawRad) * std::cos(pitchRad);

    float len = std::sqrt(mForwardX * mForwardX + mForwardY * mForwardY + mForwardZ * mForwardZ);
    if (len > 0.00001f) {
        mForwardX /= len;
        mForwardY /= len;
        mForwardZ /= len;
    }

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
    // 🔧 FIX: Near plane MUY pequeño para evitar clipping cercano
    // Far plane generoso para ver todo el modelo
    float nearPlane = 0.01f;  // Fijo y muy cercano
    float farPlane = std::max(10000.0f, mSceneSize * 500.0f);  // Muy lejano

    static int logCount = 0;
    if (logCount < 3) {  // Solo primeros 3 frames
        logCount++;
    }

    MatPerspective(out, mFOV * (float)M_PI / 180.0f, aspect, nearPlane, farPlane);
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
    float moveSpeed = mSceneSize * 0.1f;
    float movement = amount * moveSpeed;

    mPosX += mForwardX * movement;
    mPosY += mForwardY * movement;
    mPosZ += mForwardZ * movement;
}

void Camera::GetPosition(float& x, float& y, float& z) const {
    x = mPosX;
    y = mPosY;
    z = mPosZ;
}

void Camera::FocusOnPoint(float targetX, float targetY, float targetZ, float distance) {
    std::cout << "\n=== FocusOnPoint ===" << std::endl;
    std::cout << "Target: (" << targetX << ", " << targetY << ", " << targetZ << ")" << std::endl;
    std::cout << "Distance requested: " << distance << std::endl;

    // 🔧 FIX: Distancia mínima más generosa
    float minDistance = mSceneSize * 2.5f;  // <--- CAMBIADO de 1.5f a 2.5f
    if (distance < minDistance) {
        distance = minDistance;
        std::cout << "Distance adjusted to: " << distance << std::endl;
    }

    // 🔧 CAMBIO: Ángulo más alto para ver mejor el modelo (isométrico clásico)
    float angleH = 45.0f * (float)M_PI / 180.0f;   // 45° horizontal
    float angleV = 35.0f * (float)M_PI / 180.0f;   // 35° vertical (más alto que 30°)

    float offsetX = distance * std::cos(angleV) * std::cos(angleH);
    float offsetY = distance * std::sin(angleV);
    float offsetZ = distance * std::cos(angleV) * std::sin(angleH);

    mPosX = targetX + offsetX;
    mPosY = targetY + offsetY;
    mPosZ = targetZ + offsetZ;

    std::cout << "Camera positioned at: (" << mPosX << ", " << mPosY << ", " << mPosZ << ")" << std::endl;

    // Mirar exactamente al target
    LookAt(targetX, targetY, targetZ);

    std::cout << "Camera yaw: " << mYaw << ", pitch: " << mPitch << std::endl;

    // Verificar distancia real
    float dx = mPosX - targetX;
    float dy = mPosY - targetY;
    float dz = mPosZ - targetZ;
    float actualDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    std::cout << "Actual distance: " << actualDistance << std::endl;

    // 🔧 NUEVO: Verificar que el target está dentro del frustum
    float nearPlane = 0.1f;
    float farPlane = std::max(10000.0f, mSceneSize * 500.0f);
    std::cout << "Frustum check - Target distance: " << actualDistance
        << " (near=" << nearPlane << ", far=" << farPlane << ")" << std::endl;

    if (actualDistance < nearPlane || actualDistance > farPlane) {
        std::cout << "⚠️ WARNING: Target may be outside frustum!" << std::endl;
    }
    std::cout << std::endl;
}

void Camera::LookAt(float targetX, float targetY, float targetZ) {
    float dirX = targetX - mPosX;
    float dirY = targetY - mPosY;
    float dirZ = targetZ - mPosZ;

    float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (len < 0.00001f) return;

    dirX /= len;
    dirY /= len;
    dirZ /= len;

    mYaw = std::atan2(dirZ, dirX) * 180.0f / (float)M_PI;
    mPitch = std::asin(dirY) * 180.0f / (float)M_PI;
    mPitch = std::max(-89.0f, std::min(89.0f, mPitch));

    UpdateVectors();
}

void Camera::SetSceneSize(float size) {
    mSceneSize = size;
    // 🔧 FIX: Ajustar velocidad de cámara según tamaño del modelo
    mSpeed = size * 0.5f;
}

void Camera::MoveForward(float amount) {
    mPosX += mForwardX * amount;
    mPosY += mForwardY * amount;
    mPosZ += mForwardZ * amount;
}
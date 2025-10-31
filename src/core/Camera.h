#pragma once

class Camera {
public:
    Camera();

    void Update(float deltaTime);
    void GetViewMatrix(float out[16]) const;
    void GetProjectionMatrix(float out[16], float aspect) const;

    // Controles
    void SetPosition(float x, float y, float z);
    void Rotate(float yaw, float pitch);
    void Zoom(float amount);

    // Getters
    void GetPosition(float& x, float& y, float& z) const;

    // NUEVO: Focus en un punto
    void FocusOnPoint(float x, float y, float z, float distance);
    void LookAt(float x, float y, float z);

private:
    float mPosX, mPosY, mPosZ;
    float mYaw;
    float mPitch;
    float mSpeed;
    float mSensitivity;
    float mFOV;

    void UpdateVectors();
    float mForwardX, mForwardY, mForwardZ;
    float mRightX, mRightY, mRightZ;
    float mUpX, mUpY, mUpZ;
};
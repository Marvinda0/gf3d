#include "simple_logger.h"

#include "quaternion.h"

// Done with the help of "aderoian" on GitHub

Quaternion quaternion_create(float x, float y, float z, float w) {
    return gfc_vector4d(x, y, z, w);
}

void quaternion_copy(Quaternion* dest, Quaternion source) {
    gfc_vector4d_copy((*dest), source);
}

void quaternion_identity(Quaternion* q) {
    q->x = 0;
    q->y = 0;
    q->z = 0;
    q->w = 1;
}

void quaternion_add(Quaternion* dest, Quaternion a, Quaternion b) {
    dest->x = a.x + b.x;
    dest->y = a.y + b.y;
    dest->z = a.z + b.z;
    dest->w = a.w + b.w;
}

void quaternion_subtract(Quaternion* dest, Quaternion a, Quaternion b) {
    dest->x = a.x - b.x;
    dest->y = a.y - b.y;
    dest->z = a.z - b.z;
    dest->w = a.w - b.w;
}

void quaternion_conjugate(Quaternion* dest, Quaternion q) {
    dest->x = -q.x;
    dest->y = -q.y;
    dest->z = -q.z;
    dest->w = q.w;
}

void quaternion_normalize(Quaternion* q) {
    gfc_vector4d_normalize(q);
}

void quaternion_multiply_q(Quaternion* dest, Quaternion a, Quaternion b) {
    dest->x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    dest->y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    dest->z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    dest->w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
}

void quaternion_multiply_s(Quaternion* dest, Quaternion q, float s) {
    dest->x = q.x * s;
    dest->y = q.y * s;
    dest->z = q.z * s;
    dest->w = q.w * s;
}

void quaternion_multiply_v(Quaternion* dest, Quaternion q, GFC_Vector3D v) {
    dest->x = q.w * v.x + q.x * v.z - q.z * v.y;
    dest->y = q.w * v.y + q.y * v.x - q.x * v.z;
    dest->z = q.w * v.z + q.z * v.y - q.y * v.x;
    dest->w = -(q.x * v.x + q.y * v.y + q.z * v.z);
}

void quaternion_divide_s(Quaternion* dest, Quaternion q, float s) {
    dest->x = q.x / s;
    dest->y = q.y / s;
    dest->z = q.z / s;
    dest->w = q.w / s;
}

void quaternion_rotate(Quaternion* q, GFC_Vector3D axis, float a) {
    Quaternion r = { 0 };
    float half, s;
    gfc_vector3d_normalize(&axis);

    // Q_rot = q * r

    half = a * 0.5f;
    s = sinf(half);
    r.x = axis.x * s;
    r.y = axis.y * s;
    r.z = axis.z * s;
    r.w = cosf(half);

    quaternion_multiply_q(q, *q, r);
}

void quaternion_rotate_q(Quaternion* dest, Quaternion a, Quaternion b) {
    Quaternion a_conj = a, tmp;
    quaternion_conjugate(&a_conj, a_conj);
    quaternion_multiply_q(&tmp, a, b);
    quaternion_multiply_q(dest, tmp, a_conj);
}

void quaternion_rotate_v(GFC_Vector3D* dest, Quaternion q, GFC_Vector3D v) {
    quaternion_normalize(&q);

    Quaternion vq = { v.x, v.y, v.z, 0.0f }, q_conj, tmp;
    quaternion_conjugate(&q_conj, q);
    quaternion_multiply_q(&tmp, q, vq);       // tmp = q * v
    quaternion_multiply_q(&tmp, tmp, q_conj); // tmp = tmp * q*

    dest->x = tmp.x;
    dest->y = tmp.y;
    dest->z = tmp.z;
}

void quaternion_from_axis_angle(Quaternion* dest, GFC_Vector3D axis, float a) {
    float half, s;
    gfc_vector3d_normalize(&axis);
    half = a * 0.5f;
    s = sinf(half);
    dest->x = axis.x * s;
    dest->y = axis.y * s;
    dest->z = axis.z * s;
    dest->w = cosf(half);
}

void quaternion_from_euler_angles(Quaternion* dest, float x, float y, float z) {
    Quaternion yaw, pitch, roll;
    quaternion_from_axis_angle(&pitch, gfc_vector3d(1, 0, 1), x);
    quaternion_from_axis_angle(&roll, gfc_vector3d(0, 1, 0), y);
    quaternion_from_axis_angle(&yaw, gfc_vector3d(0, 0, 1), z);
    *dest = yaw;
    quaternion_multiply_q(dest, *dest, pitch);
    quaternion_multiply_q(dest, *dest, roll);
}

void quaternion_euler_angles_from(GFC_Vector3D* out, Quaternion q)
{
    // q = (x, y, z, w)
    float sinr_cosp, cosr_cosp, sinp, siny_cosp, cosy_cosp;
    sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    out->x = atan2f(sinr_cosp, cosr_cosp);

    sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1)
        out->y = copysignf(M_PI / 2.0f, sinp);
    else
        out->y = asinf(sinp);

    siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    out->z = atan2f(siny_cosp, cosy_cosp);
}

// Done with the help of "aderoian" on GitHub
#ifndef QUATERNION_H
#define QUATERNION_H

#include "gfc_vector.h"

typedef GFC_Vector4D Quaternion;

/**
 * @brief Creates a quaternion from components.
 * @param x [in] X component
 * @param y [in] Y component
 * @param z [in] Z component
 * @param w [in] W component
 * @return The constructed quaternion
 *
 * @example
 * Quaternion q = quaternion_create(0.0f, 0.0f, 0.0f, 1.0f); // Identity quaternion
 */
Quaternion quaternion_create(float x, float y, float z, float w);

/**
 * @brief Copies one quaternion to another quaternion.
 * @param dest [out] The quaternion to copy to
 * @param source [in] The quaternion to copy from
 *
 * @example
 * Quaternion a = quaternion_create(0,0,0,1);
 * Quaternion b;
 * quaternion_copy(&b, a); // b now equals a
 */
void quaternion_copy(Quaternion* dest, Quaternion source);

/**
 * @brief Sets a quaternion to an identity quaternion.
 * @param q [in/out] The quaternion to set
 *
 * @example
 * Quaternion q;
 * quaternion_identity(&q); // q is now (0,0,0,1)
 */
void quaternion_identity(Quaternion* q);

/**
 * @brief Adds two quaternions component-wise.
 * @param dest [out] The quaternion to store the result
 * @param a [in] The first quaternion
 * @param b [in] The second quaternion
 *
 * @example
 * Quaternion q1 = quaternion_create(1,2,3,4);
 * Quaternion q2 = quaternion_create(0.5f,0.5f,0.5f,0.5f);
 * Quaternion result;
 * quaternion_add(&result, q1, q2);
 */
void quaternion_add(Quaternion* dest, Quaternion a, Quaternion b);

/**
 * @brief Subtracts quaternion b from quaternion a component-wise.
 * @param dest [out] The quaternion to store the result
 * @param a [in] The quaternion to subtract from
 * @param b [in] The quaternion to subtract
 *
 * @example
 * Quaternion q1 = quaternion_create(1,1,1,1);
 * Quaternion q2 = quaternion_create(0.5f,0,0,0);
 * Quaternion result;
 * quaternion_subtract(&result, q1, q2);
 */
void quaternion_subtract(Quaternion* dest, Quaternion a, Quaternion b);

/**
 * @brief Computes the conjugate of a quaternion.
 * @param dest [out] The quaternion to store the conjugate
 * @param q [in] The quaternion to conjugate
 *
 * @example
 * Quaternion q = quaternion_create(1,2,3,4);
 * Quaternion q_conj;
 * quaternion_conjugate(&q_conj, q);
 */
void quaternion_conjugate(Quaternion* dest, Quaternion q);

/**
 * @brief Normalizes a quaternion to unit length.
 * @param q [in/out] The quaternion to normalize
 *
 * @example
 * Quaternion q = quaternion_create(1,2,3,4);
 * quaternion_normalize(&q); // q is now a unit quaternion
 */
void quaternion_normalize(Quaternion* q);

/**
 * @brief Multiplies two quaternions (Hamilton product).
 * @param dest [out] The quaternion to store the result
 * @param a [in] The first quaternion
 * @param b [in] The second quaternion
 *
 * @example
 * Quaternion q1 = quaternion_from_axis_angle(axis, angle1);
 * Quaternion q2 = quaternion_from_axis_angle(axis, angle2);
 * Quaternion combined;
 * quaternion_multiply_q(&combined, q1, q2); // combined rotation
 */
void quaternion_multiply_q(Quaternion* dest, Quaternion a, Quaternion b);

/**
 * @brief Multiplies a quaternion by a scalar.
 * @param dest [out] The quaternion to store the result
 * @param q [in] The quaternion to scale
 * @param s [in] The scalar value
 *
 * @example
 * Quaternion q = quaternion_create(1,1,1,1);
 * quaternion_multiply_s(&q, q, 0.5f);
 */
void quaternion_multiply_s(Quaternion* dest, Quaternion q, float s);

/**
 * @brief Multiplies a quaternion by a 3D vector (treats vector as quaternion with w=0).
 * @param dest [out] The quaternion to store the result
 * @param q [in] The quaternion
 * @param v [in] The vector to multiply
 *
 * @example
 * GFC_Vector3D v = {1,0,0};
 * Quaternion q = quaternion_from_axis_angle(axis, angle);
 * Quaternion result;
 * quaternion_multiply_v(&result, q, v);
 */
void quaternion_multiply_v(Quaternion* dest, Quaternion q, GFC_Vector3D v);

/**
 * @brief Divides a quaternion by a scalar.
 * @param dest [out] The quaternion to store the result
 * @param q [in] The quaternion
 * @param s [in] The scalar value
 *
 * @example
 * Quaternion q = quaternion_create(2,2,2,2);
 * quaternion_divide_s(&q, q, 2.0f); // q = (1,1,1,1)
 */
void quaternion_divide_s(Quaternion* dest, Quaternion q, float s);

void quaternion_rotate(Quaternion* q, GFC_Vector3D axis, float a);

/**
 * @brief Rotates quaternion a by quaternion b.
 * @param dest [out] The quaternion to store the rotated result
 * @param a [in] The quaternion to rotate
 * @param b [in] The rotation quaternion
 *
 * @example
 * Quaternion q = quaternion_from_axis_angle(axis1, angle1);
 * Quaternion r = quaternion_from_axis_angle(axis2, angle2);
 * Quaternion rotated;
 * quaternion_rotate_q(&rotated, q, r); // q rotated by r
 */
void quaternion_rotate_q(Quaternion* dest, Quaternion a, Quaternion b);

/**
 * @brief Rotates a 3D vector by a quaternion.
 * @param dest [out] The vector to store the rotated result
 * @param a [in] The rotation quaternion
 * @param v [in] The vector to rotate
 *
 * @example
 * GFC_Vector3D v = {1,0,0};
 * Quaternion q = quaternion_from_axis_angle(axis, angle);
 * GFC_Vector3D rotated;
 * quaternion_rotate_v(&rotated, q, v); // v rotated by q
 */
void quaternion_rotate_v(GFC_Vector3D* dest, Quaternion a, GFC_Vector3D v);

/**
 * @brief Creates a quaternion from an axis and rotation angle.
 * @param dest [out] The quaternion to store the result
 * @param axis [in] The axis of rotation (should be normalized)
 * @param a [in] The rotation angle in radians
 *
 * @example
 * GFC_Vector3D axis = {0,1,0};
 * Quaternion q;
 * quaternion_from_axis_angle(&q, axis, GFC_DEGTORAD * 90.0f);
 */
void quaternion_from_axis_angle(Quaternion* dest, GFC_Vector3D axis, float a);

/**
 * @brief Creates a quaternion from Euler angles (pitch, yaw, roll).
 * @param dest [out] The quaternion to store the result
 * @param x [in] Rotation around X-axis in radians (pitch)
 * @param y [in] Rotation around Y-axis in radians (yaw)
 * @param z [in] Rotation around Z-axis in radians (roll)
 *
 * @example
 * Quaternion q;
 * quaternion_from_euler_angles(&q, pitch, yaw, roll);
 */
void quaternion_from_euler_angles(Quaternion* dest, float x, float y, float z);

void quaternion_euler_angles_from(GFC_Vector3D* out, Quaternion q);
#endif

// Done with the help of "aderoian" on GitHub

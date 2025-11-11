#include "simple_logger.h"

#include "gf3d_vgraphics.h"
#include "gf3d_camera.h"

static Camera gf3d_camera = {0};


void gf3d_camera_update_view()
{
    GFC_Vector3D xaxis,yaxis,zaxis,position;
    float cosPitch = cos(gf3d_camera.rotation.x);
    float sinPitch = sin(gf3d_camera.rotation.x);
    float cosYaw = cos(gf3d_camera.rotation.z);
    float sinYaw = sin(gf3d_camera.rotation.z); 

    position.x = gf3d_camera.position.x;
    position.y = -gf3d_camera.position.z;        //inverting for Z-up
    position.z = gf3d_camera.position.y;
    gfc_matrix4_identity(gf3d_camera.cameraMat);

    gfc_vector3d_set(xaxis, cosYaw,                     0,  -sinYaw);
    gfc_vector3d_set(yaxis, sinYaw * sinPitch,   cosPitch,   cosYaw * sinPitch);
    gfc_vector3d_set(zaxis, sinYaw * cosPitch,  -sinPitch,   cosPitch * cosYaw);
    
    gf3d_camera.cameraMat[0][0] = xaxis.x;
    gf3d_camera.cameraMat[0][1] = yaxis.x;
    gf3d_camera.cameraMat[0][2] = zaxis.x;

    gf3d_camera.cameraMat[1][0] = xaxis.z;
    gf3d_camera.cameraMat[1][1] = yaxis.z;
    gf3d_camera.cameraMat[1][2] = zaxis.z;

    gf3d_camera.cameraMat[2][0] = -xaxis.y;
    gf3d_camera.cameraMat[2][1] = -yaxis.y;
    gf3d_camera.cameraMat[2][2] = -zaxis.y;

    gf3d_camera.cameraMat[3][0] = gfc_vector3d_dot_product(xaxis, position);
    gf3d_camera.cameraMat[3][1] = gfc_vector3d_dot_product(yaxis, position);
    gf3d_camera.cameraMat[3][2] = gfc_vector3d_dot_product(zaxis, position);
    gf3d_vgraphics_set_view(gf3d_camera.cameraMat);
}

void gf3d_camera_set_rotation_q(Quaternion rotation) {
    gf3d_camera.qRotation = rotation;
}

void gf3d_camera_update_view_q() //This Function is from "adorioan" on GitHub
{
    GFC_Vector3D right, up, forward;
    GFC_Vector3D position = gf3d_camera.position;

    // Build orientation basis from quaternion
    GFC_Vector3D localRight = gfc_vector3d(1, 0, 0);
    GFC_Vector3D localUp = gfc_vector3d(0, 0, 1);  // Z-up
    GFC_Vector3D localForward = gfc_vector3d(0, 1, 0);  // +Y forward

    quaternion_rotate_v(&right, gf3d_camera.qRotation, localRight);
    quaternion_rotate_v(&up, gf3d_camera.qRotation, localUp);
    quaternion_rotate_v(&forward, gf3d_camera.qRotation, localForward);

    // Initialize view matrix
    gfc_matrix4_identity(gf3d_camera.cameraMat);

    // Orientation (transpose of rotation matrix)
    gf3d_camera.cameraMat[0][0] = right.x;
    gf3d_camera.cameraMat[1][0] = right.y;
    gf3d_camera.cameraMat[2][0] = right.z;

    gf3d_camera.cameraMat[0][1] = up.x;
    gf3d_camera.cameraMat[1][1] = up.y;
    gf3d_camera.cameraMat[2][1] = up.z;

    gf3d_camera.cameraMat[0][2] = -forward.x;
    gf3d_camera.cameraMat[1][2] = -forward.y;
    gf3d_camera.cameraMat[2][2] = -forward.z;

    // Translation (negative dot with position)
    gf3d_camera.cameraMat[3][0] = gfc_vector3d_dot_product(right, position);
    gf3d_camera.cameraMat[3][1] = gfc_vector3d_dot_product(up, position);
    gf3d_camera.cameraMat[3][2] = -gfc_vector3d_dot_product(forward, position); // note the sign flip

    // Send to graphics
    gf3d_vgraphics_set_view(gf3d_camera.cameraMat);
}

GFC_Vector3D gf3d_camera_get_position()
{
    GFC_Vector3D position;
    gfc_vector3d_negate(position,gf3d_camera.position);
    return position;
}

void gf3d_camera_set_position(GFC_Vector3D position)
{
    gf3d_camera.position.x = -position.x;
    gf3d_camera.position.y = -position.y;
    gf3d_camera.position.z = -position.z;
}

void gf3d_camera_set_rotation(GFC_Vector3D rotation)
{
    gfc_angle_clamp_radians(&rotation.x);
    gf3d_camera.rotation.x = -rotation.x;
    gf3d_camera.rotation.y = -rotation.y;
    gf3d_camera.rotation.z = -rotation.z;
}

void gf3d_camera_set_scale(GFC_Vector3D scale)
{
    if (!scale.x)gf3d_camera.scale.x = 0;
    else gf3d_camera.scale.x = 1/scale.x;
    if (!scale.y)gf3d_camera.scale.y = 0;
    else gf3d_camera.scale.y = 1/scale.y;
    if (!scale.z)gf3d_camera.scale.z = 0;
    else gf3d_camera.scale.z = 1/scale.z;
}

void gf3d_camera_look_at(GFC_Vector3D target,const GFC_Vector3D *position)
{
    GFC_Vector3D angles,pos;
    GFC_Vector3D delta;
    if (position)
    {
        gfc_vector3d_copy(pos,(*position));
        gf3d_camera_set_position(pos);
    }
    else
    {
        pos = gf3d_camera_get_position();
    }
    gf3d_camera.lookTargetPosition = target;
    gfc_vector3d_sub(delta,target,pos);
    gfc_vector3d_angles (delta, &angles);
    angles.z -= GFC_HALF_PI;
    angles.x -= GFC_PI;
    gf3d_camera_set_rotation(angles);
}


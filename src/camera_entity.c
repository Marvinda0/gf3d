#include "simple_logger.h"
#include "gfc_input.h"
#include "gf3d_camera.h"
#include "camera_entity.h"

typedef struct
{
		GFC_Vector3D target;
};


//camera entity free (self)
/*
data
if not self or nor data in self return
data = to self data5
//where the data gets cleaned up
free data

*/

//camera entity think (self)
/* Input staff
* define move 5 turn 01 and pitch 0
* float yaw 0
* 
* 3d vector movement = 0
* 3d vector dir = 0
* init camera data data pointer
sanity check
data = to self data

vecntor sub, (dir,data target, self position)
normalize &dir

if (gather input) etc

scale dir, movement.y //before paning, after walk //move-yaw
add data tarrget, target movement

normalize dir
rotate &dir by HalfPI

add vector (self psoiton + self->position,movement)
add vector (data + self->position,movement)
if pitch
	data targetz += piych
if yaw
sub target to our position
normalize dir
rotate &dir yaw

*/

//camera entity spawn (position, target)
/*
* vectro 3d dir
data
self
entity new
sanity check
sata allocate array cameradatasize 1
sanity check
self->fill
vectror 3d sub (dit, target.position)
vectror 3d normalize (&dir)
vectror 3d add (data->target, positont, dir);
camera look at datatarget from my new postition self-> position
*/
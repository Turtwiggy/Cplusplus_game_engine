// #pragma once

// #include "bullet/btBulletDynamicsCommon.h"

// struct physics_simulation
// {
//     void init_physics();
//     ~physics_simulation();

//     void step_simulation(float delta_time);

// public:
//     btDiscreteDynamicsWorld* dynamicsWorld;

// private:
//     btDefaultCollisionConfiguration* collisionConfiguration;
//     btCollisionDispatcher* dispatcher;
//     btBroadphaseInterface* overlappingPairCache;
//     btSequentialImpulseConstraintSolver* solver;

//     //keep track of the shapes, we release memory at exit.
//     //make sure to re-use collision shapes among rigid bodies whenever possible!
//     btAlignedObjectArray<btCollisionShape*> collisionShapes;

//     bool init = false;
// };
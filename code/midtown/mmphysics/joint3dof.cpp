/*
    Open1560 - An Open Source Re-Implementation of Midtown Madness 1 Beta
    Copyright (C) 2020 Brick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

define_dummy_symbol(mmphysics_joint3dof);

#include "joint3dof.h"

#include "inertia.h"

#include "vector7/matrix34.h"

void Joint3Dof::MoveICS()
{
    // TODO: Why was LinearPush applied here?
    // It is already applied in Joint3Dof::Update.
    // if (!(JointFlags & JOINT_FLAG_BROKEN))
    // {
    //     ICS1->ApplyPush(ICS2->LinearPush);
    //     ICS2->LinearPush = ICS1->LinearPush;
    // }

    ICS1->MoveICS();
    ICS2->MoveICS();
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
Joint3Dof::~Joint3Dof() = default;

Joint3Dof::Joint3Dof()
{
    Init();
}

void Joint3Dof::Init()
{
    // A joint with nothing set up is free to swing all the way round in both
    // directions and holds no force.
    LeanLimit1 = ARTS_PI;
    RollLimit2 = ARTS_PI;
    RollLimit1 = -ARTS_PI;
    LeanLimit2 = 1.0f;
    RollLimit3 = 1.0f;

    ForceLimit = 0.0f;
    JointFlags = 0;

    FrictionLean = {0.0f, 0.0f, 0.0f};
    FrictionRoll = {0.0f, 0.0f, 0.0f};

    Orientation1.Identity();
    Orientation2.Identity();
}

void Joint3Dof::InitJoint3Dof(asInertialCS* ics1, const Vector3& offset1, asInertialCS* ics2, const Vector3& offset2)
{
    ICS1 = ics1;
    ICS2 = ics2;

    Offset1 = offset1;
    Offset2 = offset2;

    SetPosition(ORIGIN);

    // Both bodies are now constrained by this joint, and each keeps a way back to it.
    ICS1->Constraints = ICS_CONSTRAIN_LINK;
    ICS1->Joint = this;
    ICS2->Constraints = ICS_CONSTRAIN_LINK;
    ICS2->Joint = this;
}

void Joint3Dof::SetPosition(const Vector3& position)
{
    // Move each body so that its own attachment point lands on the joint: put the
    // joint position in the matrix, then push the body back along its offset.
    ICS1->Matrix.m3 = position;

    Vector3 back1 {-Offset1.x, -Offset1.y, -Offset1.z};
    Vector3 moved1;
    moved1.Dot(back1, ICS1->Matrix);
    ICS1->Matrix.m3 = moved1;

    ICS2->Matrix.m3 = position;

    Vector3 back2 {-Offset2.x, -Offset2.y, -Offset2.z};
    Vector3 moved2;
    moved2.Dot(back2, ICS2->Matrix);
    ICS2->Matrix.m3 = moved2;

    Position = position;
}

void Joint3Dof::SetJointForceFlag()
{
    // The limit pass is only worth running when the joint has friction, or a lean
    // limit short of a half turn.
    if (FrictionLean.x == 0.0f && FrictionLean.y == 0.0f && FrictionLean.z == 0.0f && FrictionRoll.x == 0.0f &&
        FrictionRoll.y == 0.0f && FrictionRoll.z == 0.0f && !(LeanLimit1 < ARTS_PI))
        JointFlags &= ~JOINT_FLAG_LIMIT;
    else
        JointFlags |= JOINT_FLAG_LIMIT;
}

void Joint3Dof::SetRestOrientMat(const Matrix34& orient1, const Matrix34& orient2)
{
    // Only the rotation matters - the translation of a rest orientation is nothing.
    Orientation1 = orient1;
    Orientation1.m3 = {0.0f, 0.0f, 0.0f};

    Orientation2 = orient2;
    Orientation2.m3 = {0.0f, 0.0f, 0.0f};
}

void Joint3Dof::SetRollLimit(f32 arg1, f32 arg2, f32 arg3)
{
    RollLimit1 = arg1;
    RollLimit2 = arg2;
    RollLimit3 = arg3;

    SetJointForceFlag();
}

void Joint3Dof::SetLeanLimit(f32 arg1, f32 arg2)
{
    LeanLimit1 = arg1;
    LeanLimit2 = arg2;

    SetJointForceFlag();
}

void Joint3Dof::SetFrictionLean(f32 arg1, f32 arg2, f32 arg3)
{
    FrictionLean = {arg1, arg2, arg3};

    SetJointForceFlag();
}

void Joint3Dof::SetFrictionRoll(f32 arg1, f32 arg2, f32 arg3)
{
    FrictionRoll = {arg1, arg2, arg3};

    SetJointForceFlag();
}

void Joint3Dof::BreakJoint()
{
    JointFlags |= JOINT_FLAG_BROKEN;

    ICS1->Constraints &= ~ICS_CONSTRAIN_LINK;
    ICS2->Constraints &= ~ICS_CONSTRAIN_LINK;
}

void Joint3Dof::UnbreakJoint()
{
    JointFlags &= ~JOINT_FLAG_BROKEN;

    ICS1->Constraints |= ICS_CONSTRAIN_LINK;
    ICS2->Constraints |= ICS_CONSTRAIN_LINK;
}

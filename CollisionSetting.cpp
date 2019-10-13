#include "stdafx.h"

#include "CollisionSetting.h"

OpcodeSettings::OpcodeSettings() : m_PrimitiveTests(true), m_FirstContact(false),m_UseCache(true)
{
}

void OpcodeSettings::SetupCollider(Opcode::Collider& collider) const
{
	collider.SetFirstContact(m_FirstContact);
	collider.SetPrimitiveTests(m_PrimitiveTests);
	collider.SetTemporalCoherence(m_UseCache);
}
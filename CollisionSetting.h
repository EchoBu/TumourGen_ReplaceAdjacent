#ifndef __OPCODESETTINGS_H_INCLUDED__
#define  __OPCODESETTINGS_H_INCLUDED__

#include <OpCode/Opcode.h>
class OpcodeSettings
{
public:
	OpcodeSettings();

	void SetupCollider(Opcode::Collider& collider) const;

	bool m_PrimitiveTests;
	bool m_FirstContact;
	bool m_UseCache;
};
#endif

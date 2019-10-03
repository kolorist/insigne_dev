#pragma once

#include <floral/stdaliases.h>

namespace stone
{
namespace tech
{

void											ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap);

struct SHData
{
	floral::vec4f								CoEffs[9];
};

class SHProbeBaker
{
public:
	SHProbeBaker();
	~SHProbeBaker();

	void										Initialize();
	void										DoBake();

};

}
}

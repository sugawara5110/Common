//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         DXR_Manager                                        **//
//**                                                                                     **//
//*****************************************************************************************//

#ifndef Class_DXR_Manager_Header
#define Class_DXR_Manager_Header

#include "../DXR/DXR_Basic.h"
#include <vector>

class Wave_DXR;
class SkinMesh_DXR;
class ParticleData_DXR;
class MeshData_DXR;
class PolygonData_DXR;

class DXR_Manager {

private:
	friend Wave_DXR;
	friend SkinMesh_DXR;
	friend ParticleData_DXR;
	friend MeshData_DXR;
	friend PolygonData_DXR;

	struct ParameterSet {
		ParameterDXR* pdx = nullptr;
		std::unique_ptr<MaterialType[]> type = nullptr;
		std::unique_ptr<int[]> EMISSIVE_NO = nullptr;
		int NumEmissive = 0;
		int NumMaterial = 0;
		int* insNum = nullptr;//アドレスを渡しておく(無い場合{1,1}の配列を渡す)

		void create(int numMaterial, int numEmissive) {
			NumMaterial = numMaterial;
			NumEmissive = numEmissive;
			type = std::make_unique<MaterialType[]>(NumMaterial);
			for (int i = 0; i < NumMaterial; i++)type[i] = NONREFLECTION;
			EMISSIVE_NO = std::make_unique<int[]>(NumMaterial);

			DXR_Manager::num_pdx++;
			DXR_Manager::num_type += NumMaterial;
		}
		~ParameterSet() {
			DXR_Manager::num_pdx--;
			DXR_Manager::num_type -= NumMaterial;
		}
	};

	static DXR_Basic* dxr;
	static int num_pdx;
	static int num_type;
	static std::vector<ParameterSet*> pset;
	static std::unique_ptr<ParameterDXR* []> Pdx;
	static std::unique_ptr<MaterialType[]> Type;

	DXR_Manager() {}
	static void setParameterDXR(ParameterSet* ps, ParameterDXR* pdx);
	static void psetErase(int numMesh, ParameterSet* psArr);
	static void EmissiveNoUpdate();

public:
	static void createDxrParameter(UINT maxRecursion);
	static DXR_Basic* getInstance() { return dxr; }
	static bool PointLightPosSet(int Idx, bool on_off, float range, CoordTf::VECTOR3 atten = { 0.01f, 0.001f, 0.001f });
	static void DeleteInstance();
};

#endif
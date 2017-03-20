#include "BSP.h"

#define CONTENTS_SOLID	0x1

BSP::BSP(void)
{
	pMapData = NULL;
	mapName = NULL;
	hFile = NULL;
}

BSP::~BSP(void)
{
	if (pMapData != NULL)
		free(pMapData);
	if (mapName != NULL)
		free(mapName);
}

HANDLE BSP::loadFile(std::string path, DWORD &size)
{

	hFile = CreateFile(path.c_str(), GENERIC_READ, NULL, NULL, OPEN_ALWAYS, NULL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	size = GetFileSize(hFile, NULL);

	if (!size)
	{
		CloseHandle(hFile);
		return NULL;
	}
	else
		return hFile;
}

dheader_t* BSP::LoadBSP(const std::string& path) {

	delete[] pMapData;
	pMapData = NULL;
	mapName = NULL;
	hFile = NULL;

	if (!(hFile = loadFile(path, size)))
		return NULL;

	if (pMapData != NULL)
		delete[] pMapData;

	pMapData = new BYTE[size];

	DWORD dwReadSize = NULL;

	if (!ReadFile(hFile, pMapData, size, &dwReadSize, NULL)) {
		CloseHandle(hFile);
		delete[] pMapData;
		pMapData = NULL;
		return nullptr;
	}

	fileData = (dheader_t*)pMapData;

	nodeLump = getNodeArray();
	planeLump = getPlaneArray();
	leafLump = getLeafArray();

	CloseHandle(hFile);

	return (dheader_t*)pMapData;
}

dnode_t * BSP::getNodeArray(void)
{
	int offset = fileData->lumps[5].fileofs;
	byte * node = (byte*)fileData;
	node += offset;
	return (dnode_t*)node;
}

dplane_t * BSP::getPlaneArray(void)
{
	int offset = fileData->lumps[1].fileofs;
	byte * plane = (byte*)fileData;
	plane += offset;
	return (dplane_t*)plane;
}

dleaf_t * BSP::getLeafArray(void)
{
	int offset = fileData->lumps[10].fileofs;
	byte * leaf = (byte*)fileData;
	leaf += offset;
	return (dleaf_t*)leaf;
}

dleaf_t* BSP::GetLeafForPoint(D3DXVECTOR3& point) {

	int node = 0;

	dnode_t* pNode;
	dplane_t* pPlane;

	float d = 0.0f;

	while (node >= 0) {
		pNode = &nodeLump[node];
		pPlane = &planeLump[pNode->planenum];

		d = (point.x * pPlane->normal.x + point.y * pPlane->normal.y + point.z * pPlane->normal.z) - pPlane->dist;

		node = (d > 0) ? pNode->children[0] : pNode->children[1];
	}

	return &leafLump[-node - 1];
}

bool BSP::Visible(float* vStart, float* vEnd) {
	dheader_t* fileData = (dheader_t*)pMapData;
	D3DXVECTOR3 vDirection;
	vDirection.x = vEnd[0] - vStart[0];
	vDirection.y = vEnd[1] - vStart[1];
	vDirection.z = vEnd[2] - vStart[2];


	int iStepCount = (int)(sqrt(vDirection.x * vDirection.x + vDirection.y *
		vDirection.y + vDirection.z * vDirection.z));

	vDirection.x /= iStepCount;
	vDirection.y /= iStepCount;
	vDirection.z /= iStepCount;

	dleaf_t* pLeaf = nullptr;

	int i = 0;
	D3DXVECTOR3 vPoint = { vEnd[0], vEnd[1], vEnd[2] };

	while (i <= iStepCount) {

		vPoint.x -= vDirection.x;
		vPoint.y -= vDirection.y;
		vPoint.z -= vDirection.z;

		pLeaf = GetLeafForPoint(vPoint);

		if (pLeaf) {
			if (pLeaf->contents & CONTENTS_SOLID) {
				return false;
			}
		}
		i++;
	}

	return true;
}
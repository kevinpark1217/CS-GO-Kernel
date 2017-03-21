#pragma once
#include <Windows.h>
#include <string>

//Stolen from SDK
#define	HEADER_LUMPS					64
#define	MAX_MAP_NODES					65536
#define	MAX_MAP_PLANES					65536
#define	MAX_MAP_LEAFS					65536

struct D3DXVECTOR3
{
	float x, y, z;
};

struct dnode_t
{
	int		planenum;	// index into plane array
	int		children[2];	// negative numbers are -(leafs + 1), not nodes
	short		mins[3];	// for frustum culling
	short		maxs[3];
	unsigned short	firstface;	// index into face array
	unsigned short	numfaces;	// counting both sides
	short		area;		// If all leaves below this node are in the same area, then
							// this is the area index. If not, this is -1.
	short		paddding;	// pad to 32 bytes length
};

struct dleaf_t
{
	int			contents;		// OR of all brushes (not needed?)
	short			cluster;		// cluster this leaf is in
	short			area : 9;			// area this leaf is in
	short			flags : 7;		// flags
	short			mins[3];		// for frustum culling
	short			maxs[3];
	unsigned short		firstleafface;		// index into leaffaces
	unsigned short		numleaffaces;
	unsigned short		firstleafbrush;		// index into leafbrushes
	unsigned short		numleafbrushes;
	short			leafWaterDataID;	// -1 for not in water

										//!!! NOTE: for maps of version 19 or lower uncomment this block
										/*
										CompressedLightCube	ambientLighting;	// Precaculated light info for entities.
										short			padding;		// padding to 4-byte boundary
										*/
};


struct dplane_t
{
	D3DXVECTOR3	normal;	// normal vector
	float	dist;	// distance from origin
	int	type;	// plane axis identifier
};

struct lump_t
{
	int		fileofs, filelen;
	int		version;		// default to zero
	char	fourCC[4];		// default to ( char )0, ( char )0, ( char )0, ( char )0
};

struct dheader_t
{
	int	ident;                // BSP file identifier
	int	version;              // BSP file version
	lump_t	lumps[HEADER_LUMPS];  // lump directory array
	int	mapRevision;          // the map's revision (iteration, version) number
};


class BSP {

public:

	BSP(void);
	~BSP(void);

	dheader_t* LoadBSP(const std::string& path);

	dnode_t * getNodeArray(void);

	dplane_t * getPlaneArray(void);

	dleaf_t * getLeafArray(void);

	dleaf_t* GetLeafForPoint(D3DXVECTOR3& point);

	bool Visible(float* vStart, float* vEnd);

	HANDLE loadFile(std::string path, DWORD &size);



private:
	byte * pMapData;
	char * mapName;
	DWORD size;
	HANDLE hFile;
	dheader_t * fileData;

	dnode_t * nodeLump;
	dplane_t * planeLump;
	dleaf_t * leafLump;

};
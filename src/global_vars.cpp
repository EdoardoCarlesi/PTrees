/*
 *   METROC++: MErger TRees On C++, a scalable code for the computation of merger trees in cosmological simulations.
 *   Copyright (C) Edoardo Carlesi 2018-2019
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


/* 
 * global_vars.cpp: 
 * A collection of variables and structures that are being use throughout the program.
 */

#include <map>
#include <mpi.h>
#include <vector>

#include "Halo.h"
#include "Grid.h"
#include "global_vars.h"

using namespace std;

/*
 *	General variables declared here and used program-wide for synchronization
 */
int locTask;
int totTask;
MPI_Request request_recv, request_send;
MPI_Status status;

#ifndef ZOOM
Grid GlobalGrid[2];
#endif

vector<vector<MergerTree>> locMTrees;
vector<vector<MergerTree>> locCleanTrees;

/* We keep track of the orphan halos to synchronize them afterwards */
vector<int> orphanHaloIndex;	// Global index

vector<vector<Halo>> allHalos;
vector<vector<Halo>> locHalos;
vector<vector<vector<vector<unsigned long long int>>>> locParts;

#ifndef ZOOM
vector<Halo> locBuffHalos;
vector<Halo> locOrphHalos;
vector<int> locOrphIndex;
vector<vector<vector<unsigned long long int>>> locOrphParts;
vector<vector<vector<unsigned long long int>>> locBuffParts;
#else
vector<int> locTreeIndex;
#endif

#ifdef CMP_MAP
typedef struct Particle Particle;
vector<map <unsigned long long int, vector<Particle>>> locMapParts;

map <unsigned long long int, int> thisMapTrees;
map <unsigned long long int, int> nextMapTrees;
#endif

size_t locHalosSize[2];

int iNumCat; // Halo catalog number in use, from 0 to N
int iUseCat; // Refers to 0 or 1 depending on the snapshot being used
int nTypePart = NPTYPES;
int nTotHalos[2];
int nLocHalos[2];

size_t locPartsSize[2];
size_t sizeHalo;
size_t sizePart;

int nPTypes;
int nLocParts[2];

/* Maps that contain halo ids and a link to the locHalo index */
#ifndef ZOOM
map <unsigned long long int, int> locId2Index;
#endif
map <unsigned long long int, int> id2Index;

/* Factors and variables */
float dMaxFactor;
float totVmax;
float locVmax;
float maxBufferThick;
float boxSize;

string cosmologicalModel;

int minPartCmp;
int minPartHalo;

int runMode;
int nGrid;
int facOrphanSteps;
int nTreeChunks;
int nLocChunks;
int nChunks;
int nSnapsUse;
int nSnaps;

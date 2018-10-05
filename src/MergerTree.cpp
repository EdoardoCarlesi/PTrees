#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

#include "MergerTree.h"
#include "Halo.h"

#include "utils.h"
#include "global_vars.h"

using namespace std;



HaloTree::HaloTree()
{
};


HaloTree::~HaloTree()
{
	Clean();
};


void HaloTree::Clean()
{
	for (int iM = 0; iM < mTree.size(); iM++)
		mTree[iM].Clean();

	mTree.clear();
	mainHalo.clear();
};


void MergerTree::Info()
{
	cout << "Task=" << locTask << " " << mainHalo.ID << " " << idProgenitor.size() << endl;

	if (tokenProgenitor)
		for (int iP = 0; iP < idProgenitor.size(); iP++)
			cout << "Ind=" << indexProgenitor[iP] << " ID= " << idProgenitor[iP] << endl;
		else
			for (int iP = 0; iP < idProgenitor.size(); iP++)
				cout << "Ind=" << indexProgenitor[iP] << " ID= " << idProgenitor[iP] << " NP=" << nCommon[1][iP] << endl;

};



void MergerTree::Clean()
{
	nCommon.clear();
	idProgenitor.clear();
	indexProgenitor.clear();
};



MergerTree::MergerTree()
{
	//tokenProgenitor = false;
};



MergerTree::~MergerTree()
{
};


/* This sorting algorithm might be very inefficient but it's straightforward to implement, 
 * plus we will rarely deal with halos with more than 10^6 progenitors */
void MergerTree::sortByMerit(int jSimu)
{
	vector<unsigned long long int> tmpIdx;
	vector<float> allMerit;
	vector<int> idx, tmpIndex;
	//vector<Halo> tmpSubHalos;

	int iSimu = (jSimu + 1) % 2; 
	float merit = 0.0;

	for (int iM = 0; iM < idProgenitor.size(); iM++)
	{
		int thisIndex = indexProgenitor[iM];
		Halo thisProgHalo = locHalos[iSimu][thisIndex];
		int nComm = 0;

		for(int iC = 0; iC < nPTypes; iC++)
				nComm += nCommon[iC][iM];

		merit = ((float) nComm * nComm) / (mainHalo.nPart[1] * thisProgHalo.nPart[1]);
		merit *= merit;

		//cout << iM << " " << merit << " " << thisProgHalo.mTot << " " << endl;

		allMerit.push_back(merit);
	}
	
	idx = SortIndexes(allMerit);
	tmpIndex.resize(idx.size());
	tmpIdx.resize(idx.size());
	//tmpSubHalos.resize(idx.size());

	for (int iM = 0; iM < idProgenitor.size(); iM++)
	{
		//cout << iM << ", " << idx[iM] << ", " << idProgenitor[iM] << ", " << idProgenitor.size() << endl;
		tmpIdx[iM] = idProgenitor[idx[iM]];
		tmpIndex[iM] = indexProgenitor[idx[iM]];
		//tmpSubHalos[iM] = subHalos[idx[iM]];
	}	

	idProgenitor = tmpIdx;
	indexProgenitor = tmpIndex;
	//subHalos = tmpSubHalos;

	/*	
	cout << idx << ", " << locHalos[iSimu][indexProgenitor[idx]].mTot << endl;
	cout << 0   << ", " << locHalos[iSimu][indexProgenitor[0]].mTot << endl;
	cout << "........." << endl;

	int idx = 0;
	for (int iM = 0; iM < idProgenitor.size(); iM++)
	{
		cout << locTask << "] (" << idx[iM] << ", " << iM << ") (" 
			<< allMerit[idx[iM]] << ", " << allMerit[iM] << ") (" 
			<< locHalos[iSimu][indexProgenitor[iM]].mTot << ", " 
			<< locHalos[iSimu][indexProgenitor[idx[iM]]].mTot << ") " << endl; 
	}
		cout << endl;
	}*/



};


       /************************************************************************* 
	* These are general functions and are not part of the class Merger Tree *
	*************************************************************************/
		
/* Given two halos, decide whether to compare their particle content or not */
bool CompareHalos(int iHalo, int jHalo, int iOne, int iTwo)
{
	float min = 100000.0, max = 0.0;
	float rMax = 0.0, vMax = 0.0, fVel = 0.5e-2, vOne = 0.0, vTwo = 0.0;
	Halo cmpHalo;

#ifdef ZOOM	// TODO this is not clear yet
	cmpHalo = locHalos[iTwo][jHalo];
	rMax = locHalos[iOne][iHalo].rVir + cmpHalo.rVir;
	rMax *= 25.0; 
	//rMax = locHalos[iOne][iHalo].rVir + cmpHalo.rVir; 
	//rMax *= dMaxFactor * 5.0;
	
	//cout << iHalo << " " << jHalo << " " << locHalos[iOne][iHalo].Distance(cmpHalo.X) << " " << rMax << endl;

	// Only check for pairwise distance
	//if (locHalos[iOne][iHalo].Distance(cmpHalo.X) < rMax)
	//	return true;
	//else 
	//	return false;
#else
	// TODO introduce more checks on the time, velocity and so on
	// do some check - if jHalo > nLocHalos ---> go look into the buffer halos FIXME
	if (jHalo >= 0)
		cmpHalo = locHalos[iTwo][jHalo];
	if (jHalo < 0)
		cmpHalo = locBuffHalos[-jHalo];

	rMax = locHalos[iOne][iHalo].rVir + cmpHalo.rVir; 
#endif

	vOne = VectorModule(locHalos[iOne][iHalo].V); vTwo = VectorModule(cmpHalo.V);
	vMax = (vOne + vTwo) * fVel;
	rMax *= dMaxFactor * vMax;

	// If we are dealing with token halos, enlarge the search radius
	if (locHalos[iOne][iHalo].isToken || cmpHalo.isToken)
			rMax *= 2.0;
	
	// Only check for pairwise distance
	if (locHalos[iOne][iHalo].Distance(cmpHalo.X) < rMax)
	{
		//cout << "Halo comparison: " << locHalos[iOne][iHalo].Distance(cmpHalo.X) << " r: " << rMax << endl;
		//locHalos[iOne][iHalo].Info();
		//cmpHalo.Info();
		//cout << "--" << endl;
		return true;
	} else {
		//cout << "Skipping halo comparison: " << endl;
		//locHalos[iOne][iHalo].Info();
		//cmpHalo.Info();
		//cout << "--" << endl;
		return false;
	}
	
};


/* Find all the progenitors (descendants) of the halos in catalog iOne (iTwo) */
void FindProgenitors(int iOne, int iTwo)
{
	int nStepsCounter = floor(nLocHalos[iUseCat] / 50.);
	float rSearch = 0, facRSearch = 20.0;
	vector<int> thisNCommon, indexes, totNCommon;
	int totCmp = 0, totSkip = 0; 

	locMTrees[iOne].clear();
	locMTrees[iOne].shrink_to_fit();
	locMTrees[iOne].resize(nLocHalos[iOne]);

#ifdef ZOOM
	vector<int> locHaloPos;
	int thisIndex = 0, halosPerTask = 0, halosRemaind = 0;

	/* For consistency across tasks and to keep track of all possible orphans, IDs need to be initialized here */
	for (int iT = 0; iT < locHalos[iOne].size(); iT++)
		locMTrees[iOne][iT].mainHalo.ID = locHalos[iOne][iT].ID;

	/* The way forward/backward comparisons are done is different: in the forward mode, halos are split among the tasks as 
	 * Task 1 --> halo 1, task 2 ---> halo 2, assuming that the halos are ordered with increasing (decreasing) number of particles.
	 * In the backward mode, we assign to each task only those halos which have been matched in the fwd step, to avoid unnecessary
	 * comparisons.   */
	if (iOne < iTwo)	
	{
		halosPerTask = int(nLocHalos[iOne] / totTask);
		halosRemaind = nLocHalos[iOne] % totTask;
		locTreeIndex.clear();
		locTreeIndex.shrink_to_fit();

		/* The first halosRemaind tasks get one halo more */
		if (locTask < halosRemaind)
			halosPerTask += 1;

		if (locTask == 0)
			cout << "Looping on " << halosPerTask << " halos in the forward loop." << endl;
	} else {

		/* If doing backward correlation we have to loop on all halos (ZOOM mode) */
		halosPerTask = locTreeIndex.size();
		
		if (locTask == 0)
			cout << "Looping on " << halosPerTask << " halos in the backward loop." << endl;
	}

	for (int i = 0; i < halosPerTask; i++)
	{
		/* This distribution assumes that halos are ordered per number of particles, to improve the load balancing */
		if (iOne < iTwo)
			thisIndex = locTask + i * totTask;
		else
			thisIndex = locTreeIndex[i];

		Halo thisHalo = locHalos[iOne][thisIndex];
		
		/* Save some halo properties on the mtree vector */
		locMTrees[iOne][thisIndex].mainHalo = thisHalo;
		locMTrees[iOne][thisIndex].nCommon.resize(nPTypes);
	
		//thisHalo.Info();
		//cout << iOne << ", " << i << ", " << thisHalo.ID << endl;
	
			/* In a zoom-in run, we loop over all the halos on the iTwo step */
			for (int j = 0; j < locHalos[iTwo].size(); j++)
			{ 
				thisNCommon.resize(nPTypes);
                                //if (CompareHalos(i, j, iOne, iTwo))	// TODO this does not really improve speed, need to do more tests
				{
					thisNCommon = CommonParticles(locParts[iOne][thisIndex], locParts[iTwo][j]);
	
					int totComm = 0;
					totComm = thisNCommon[0] + thisNCommon[1] + thisNCommon[2];

					/* This is very important: we keep track of the merging history ONLY if the number 
					 * of common particles is above a given threshold */
					if (totComm > minPartCmp) 
					{	
						/* Common particles are separated per particle type */	
						for (int iT = 0; iT < nPTypes; iT++)
						{	
							//cout << j << ", " << iT << ", " << thisNCommon[iT] << endl;
							locMTrees[iOne][thisIndex].nCommon[iT].push_back(thisNCommon[iT]);
						}

						locMTrees[iOne][thisIndex].idProgenitor.push_back(locHalos[iTwo][j].ID);
						locMTrees[iOne][thisIndex].indexProgenitor.push_back(j);
					
						/* If the two halos have the same ID, we are dealing with a token halo
						 * placed to trace the progenitor of an orphan halo */
						if (locMTrees[iOne][thisIndex].mainHalo.ID == locHalos[iTwo][j].ID)
							locMTrees[iOne][thisIndex].tokenProgenitor == true;

						/* We keep track of the halos on iTwo that have been matched on iOne on the 
						 * local task, so that we can avoid looping on all iTwo halos afterwards */
						if (iOne < iTwo)
							locTreeIndex.push_back(j);
					
						totCmp++;
				
					}

				} // CompareHalos
				
				thisNCommon.clear();
				thisNCommon.shrink_to_fit();
			}

			locMTrees[iOne][thisIndex].sortByMerit(iOne);

			/* Very important: if it turns out the halo has no likely progenitor, and has a number of particles above 
			 * minPartHalo, then we keep track of its position in the global locHalos[iOne] array  */
			if (locMTrees[iOne][thisIndex].idProgenitor.size() == 0 && 
				locMTrees[iOne][thisIndex].mainHalo.nPart[1] > minPartHalo && iOne < iTwo)
				{
					for (int iT = 0; iT < nPTypes; iT++)
						locMTrees[iOne][thisIndex].nCommon[iT].push_back(locHalos[iOne][thisIndex].nPart[iT]);

					orphanHaloIndex.push_back(thisIndex);
					locMTrees[iOne][thisIndex].tokenProgenitor = true;
				} else {
					locMTrees[iOne][thisIndex].tokenProgenitor = false;
				}

#ifdef DEBUG		// Sanity check
			if (locMTrees[iOne][thisIndex].nPart > 1000)
				locMTrees[iOne][thisIndex].Info();
#endif
	}	// End loop on iOne halo

	/* Sort and clean of multiple entries the locTreeIndex */
	sort(locTreeIndex.begin(), locTreeIndex.end());
	locTreeIndex.erase(unique(locTreeIndex.begin(), locTreeIndex.end()), locTreeIndex.end());

#else	/* Comparison for fullbox simulations */

		for (int i = 0; i < nLocHalos[iOne]; i++)
		{
			//locMTrees[iOne][i].idDescendant = locHalos[iOne].ID;
			locMTrees[iOne][i].mainHalo = locHalos[iOne];
			//locMTrees[iOne][i].nPart = locHalos[iOne].nPart[nPTypes];
			locMTrees[iOne][i].nCommon.resize(nPTypes);

			if (i == nStepsCounter * floor(i / nStepsCounter) && locTask == 0)
					cout << "." << flush; 

#ifndef CMP_ALL		/* Compare to a subset of halos, not to all of them */

			Halo thisHalo = locHalos[iOne][i];
			rSearch = facRSearch * thisHalo.rVir;

			/* We only loop on a subset of halos */
			indexes = GlobalGrid[iTwo].ListNearbyHalos(thisHalo.X, rSearch);

			/* The vector "indexes" contains the list of haloes (in the local memory & buffer) to be compared */
			for (int j = 0; j < indexes.size(); j++)
			{
				int k = indexes[j];

				//locMTrees[iOne][i].idDescendant = locHalos[iOne].ID;
				//locMTrees[iOne][i].nPart = locHalos[iOne].nPart[nPTypes];
				locMTrees[iOne][i].mainHalo = locHalos[iOne];
				locMTrees[iOne][i].nCommon.resize(nPTypes);

				/* Compare halos --> this functions checks whether the two halos are too far 
				   or velocities are oriented on opposite directions */
				if (CompareHalos(i, k, iOne, iTwo))
				{	
					if (k >= 0)
						nCommon = CommonParticles(locParts[iOne][i], locParts[iTwo][k]);
					else
						nCommon = CommonParticles(locParts[iOne][i], locBuffParts[-k]);

					if (nCommon[1] > 10) 
					{		
						for (int iT = 0; iT < nPTypes; iT++)
						{
							locMTrees[iOne][i].nCommon[iT].push_back(nCommon[iT]);
						}
	
					if (k >= 0)
					
						locMTrees[iOne][i].idProgenitor.push_back(locHalos[k].ID);
					else 
						locMTrees[iOne][i].idProgenitor.push_back(locBuffHalos[-k].ID);
						
					locMTrees[iOne][i].indexProgenitor.push_back(k);

						totCmp++;
					} else {
						totSkip++;
					}
				} // Halo Comparison

			}	// for j, k = index(j)

#else			/* Compare ALL the halos located on the task - used only as a benchmark */

			for (int j = 0; j < locHalos[iTwo].size(); j++)
			{
				nCommon = CommonParticles(locParts[iOne][i], locParts[iTwo][j]);
						
				if (nCommon[1] > 10) 
				{		
					for (int iT = 0; iT < nPTypes; iT++)
					{
						locMTrees[iOne][i].nCommon[iT].push_back(nCommon[iT]);
					}

					locMTrees[iOne][i].idProgenitor.push_back(locHalos[iTwo][j].ID);
					locMTrees[iOne][i].indexProgenitor.push_back(j);
					totCmp++;
				}
			}

			for (int j = 0; j < locBuffHalos.size(); j++)
			{
				nCommon = CommonParticles(locParts[iOne][i], locBuffParts[j]);
							
				if (nCommon[1] > 10) 
				{		
					for (int iT = 0; iT < nPTypes; iT++)
					{
						locMTrees[iOne][i].nCommon[iT].push_back(nCommon[iT]);
					}

					locMTrees[iOne][i].idProgenitor.push_back(locBuffHalos[j].ID);
					locMTrees[iOne][i].indexProgenitor.push_back(-j);
				}

				totCmp++;
			}


#endif		// compare all halos
		} // for i halo, the main one

#endif		// ifdef ZOOM
};


/* Given a pair of haloes, determine the number of common particles */
vector<int> CommonParticles(vector<vector<unsigned long long int>> partsHaloOne, 
	vector<vector<unsigned long long int>> partsHaloTwo)
{
	vector<int> nCommon; 
	vector<unsigned long long int>::iterator iter;
	vector<unsigned long long int> thisCommon;

	nCommon.resize(nPTypes);	

	for (int iT = 0; iT < nPTypes; iT++)
	{
		int thisSize = partsHaloOne[iT].size();

		if (thisSize > 0)
		{
			//cout << locTask << " " << iT << " " << partsHaloOne[iT][0] << " " << partsHaloOne[iT][1]
			//	<< " " << partsHaloTwo[iT][0] << " " << partsHaloTwo[iT][1] << endl;

			// This is the maximum possible number of common particles
			thisCommon.resize(thisSize);
	
			// Find out how many particles are shared among the two arrays
			iter = set_intersection(partsHaloOne[iT].begin(), partsHaloOne[iT].end(), 
				partsHaloTwo[iT].begin(), partsHaloTwo[iT].end(), thisCommon.begin());	

			// Resize the array and free some memory
			thisCommon.resize(iter - thisCommon.begin());
			//thisCommon.shrink_to_fit();		

			// Now compute how many particles in common are there
			nCommon[iT] = thisCommon.size();
		
			// Clear the vector and free all the allocated memory
			thisCommon.clear();
	 		thisCommon.shrink_to_fit();
		}
	}
	
	return nCommon;

};



void InitTrees(int nUseCat)
{
	locCleanTrees.resize(nUseCat-1);
	//allHalos.resize(nUseCat);
	//copy(locHalos[0].begin(), locHalos[0].end(), back_inserter(allHalos[iNumCat]));
};



void CleanTrees(int iStep)
{
	int thisIndex = 0, halosPerTask = 0, halosRemaind = 0;

        halosPerTask = int(nLocHalos[0] / totTask);
        halosRemaind = nLocHalos[0] % totTask;

	if (locTask < halosRemaind)
		halosPerTask += 1;

	if (locTask == 0)
		cout << "Cleaning Merger Tree connections, back and forth..." << endl;

	for (int kTree = 0; kTree < halosPerTask; kTree++)
	{
		int iTree = locTask + kTree * totTask;
		unsigned long long int mainID = locHalos[0][iTree].ID;

		MergerTree mergerTree;
		mergerTree.nCommon.resize(nPTypes);
		mergerTree.mainHalo = locHalos[0][iTree];
		mergerTree.tokenProgenitor = locMTrees[0][iTree].tokenProgenitor;
	
		/* At each step we only record the connections between halos in catalog 0 and catalog 1, without attempting at a
		 * reconstruction of the full merger history. This will be done later. */
		for (int iProg = 0; iProg < locMTrees[0][iTree].idProgenitor.size(); iProg++)
		{
			int jTree = locMTrees[0][iTree].indexProgenitor[iProg];
			unsigned long long int progID = locMTrees[0][iTree].idProgenitor[iProg];
			unsigned long long int descID = locMTrees[1][jTree].idProgenitor[0];	// The progenitor in the backwards tree
			Halo subHalo;	subHalo = locMTrees[1][jTree].mainHalo;

#ifdef DEBUG
			// Useful for debugging
			if (descID == 0)
			{
				cout << "WARNING. Progenitor ID not assigned: " << progID << " " << descID 
					<< " | " << iTree << " " << jTree << endl;

				locHalos[0][iTree].Info();
				locHalos[1][jTree].Info();
			}
#endif
			if (mainID == descID)
			{
				if (locMTrees[0][iTree].tokenProgenitor)
				{
					mergerTree.idProgenitor.push_back(locMTrees[0][iTree].mainHalo.ID);
					mergerTree.indexProgenitor.push_back(jTree);
					mergerTree.subHalos.push_back(locHalos[0][iTree]);

					for(int iT = 0; iT < nPTypes; iT++)
						mergerTree.nCommon[iT].push_back(locHalos[0][iTree].nPart[iT]);

				} else {
					mergerTree.idProgenitor.push_back(progID);
					mergerTree.indexProgenitor.push_back(jTree);
					mergerTree.subHalos.push_back(subHalo);

					for(int iT = 0; iT < nPTypes; iT++)
						mergerTree.nCommon[iT].push_back(locMTrees[0][iTree].nCommon[iT][iProg]);
				}
			}
		}	// kTree & iTree loop

		if (mergerTree.idProgenitor.size() > 0)
			locCleanTrees[iStep-1].push_back(mergerTree);

		mergerTree.Clean();
	}
	
	locTreeIndex.clear();
	locTreeIndex.shrink_to_fit();
};



void DebugTrees()
{
	if (locTask == 0)
		cout << "Debugging trees for steps=" << locCleanTrees.size() << endl;
 
	for (int iC = 0; iC < locCleanTrees.size(); iC++)
	{	
		cout << "Task=(" << locTask << ") Size=(" << locCleanTrees[iC].size() << ") Step=(" << iC << ") " << endl;

		for (int iT = 0; iT < locCleanTrees[iC].size(); iT++)
				locCleanTrees[iC][iT].Info();
	}
};




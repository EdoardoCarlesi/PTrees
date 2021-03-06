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
 * main.cpp:
 * The main file is a wrapper for all the functions that need to be invoked to 
 * compute the merger trees. 
 * First, we read an input file containing the necessary settings.
 * Then halo/particle files are distributed and read in by the tasks.
 * Buffers along the borders of the subvolume contained by each task are then computed and distributed across the tasks.
 * Halo merger trees are then computed consistently on each individual task. */

#include <mpi.h>

#include <vector>
#include <iostream>
#include <string>
#include <ctime>

#include "global_vars.h"
#include "utils.h"

#include "Communication.h"
#include "IOSettings.h"
#include "Halo.h"
#include "MergerTree.h"
#include "Cosmology.h"

using namespace std;



int main(int argv, char **argc)
{
	IOSettings SettingsIO;
	Communication CommTasks;
	Cosmology Cosmo;

	string configFile = argc[1];

	/* Read configuration file and initialize variables */
	SettingsIO.ReadConfigFile(configFile);

	/* Fire off MPI */
	MPI_Init(&argv, &argc);
	MPI_Comm_rank(MPI_COMM_WORLD, &locTask);
 	MPI_Comm_size(MPI_COMM_WORLD, &totTask);

	/* These are the local variables and I/O settings that are defined for each task*/
	InitLocVariables();

	SettingsIO.Init();

	InitTrees(nSnapsUse);
	SettingsIO.SetCosmology(&Cosmo);

	string strRunMode;

	/* We are assuming that each task reads more than one file. TODO load balancing */
	SettingsIO.DistributeFilesAmongTasks();

	if (locTask == 0)
	{
		cout << endl;
		cout << "\t\t**************************************************" << endl;
		cout << "\t\t*       METROC++ (MErger TRees On C++) v0.2      *" << endl;
		cout << "\t\t*    Copyright (C) Edoardo Carlesi, 2018-2019    *" << endl;
		cout << "\t\t* This program comes with ABSOLUTELY NO WARRANTY *" << endl;
	        cout << "\t\t* This is free software, and you are welcome to  *" << endl;
  		cout << "\t\t* redistribute it under certain conditions.      *" << endl;
		cout << "\t\t**************************************************" << endl;
		cout << endl;

		cout << endl;
		cout << "\t\t    =========================================" << endl;
#ifdef ZOOM
		cout << "\t\t    ===========  ZOOM OPERATION MODE ========" << endl;
#else
		cout << "\t\t    ========= FULL BOX OPERATION MODE =======" << endl;
#endif
		cout << "\t\t    =========================================" << endl;
		cout << endl;

		cout << "Reading " << nLocChunks << " files per task on " << totTask << " MPI tasks." << endl;
	
	}

	if (NPTYPES < 2 && locTask == 0)
	{
		cout << "\t\t==== WARNING ====" << endl;
		cout << "The code has been compiled with NPTYPES=" << NPTYPES << "." << endl;
		cout << "NPTYPES needs to be set > 2 for the code to function properly." << endl;
		cout << "Exiting..." << endl;
		cout << "\t\t=================\n" << endl;
	}
	
		/* Ready? Go! */

	{
		/* Overrides the config file settings */
		nTreeChunks = totTask;
	
		/* This is a global variable */
		iUseCat = 0;

#ifdef ZOOM	/* TODO: ZOOM mode could be just run in serial mode ... but this implies lots of code re-writing. 
		 * So let it run with only one task instead. */
		if (totTask > 1)
		{
			cout << "ZOOM mode requires only one MPI task to run correctly." << endl;	
			cout << "Please restart setting the number of tasks equal to one.\nExiting program..." << endl;
			MPI_Finalize();
			exit(0);
		}
#endif
		/* Read particles and catalogs */
		SettingsIO.ReadHalos();

#ifdef VERBOSE
		//if (locTask == 0)
		//	SettingsIO.CheckStatus();
#endif

		SettingsIO.ReadParticles();	

#ifndef ZOOM
		/* Now every task knows which subvolumes of the box belong to which task */
		CommTasks.BroadcastAndGatherGrid();
#endif

		if (locTask == 0)
		{
			/* Initialize the log time output */
			SettingsIO.WriteLog(0, 0.0);
			cout << "Starting to loop on " << nSnapsUse << " halo and particle files." << endl;
		}

		/* Loop on halo and particle catalogs */
		for (iNumCat = 1; iNumCat < nSnapsUse; iNumCat++)
		{
			clock_t iniTime = clock();
			iUseCat = 1;
			SettingsIO.ReadHalos();
			SettingsIO.ReadParticles();
		
			clock_t endTime = clock();
			double elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;
	
			if (locTask == 0)
			{
				cout << "Files read in " << elapsed << "s. " << endl;
				SettingsIO.WriteLog(iNumCat, elapsed);
			}

#ifndef ZOOM		/* No buffer exchange in zoom mode */
			iniTime = clock();

			/* Now every task knows which nodes belongs to which task */
			CommTasks.BroadcastAndGatherGrid();

			/* After reading in the second halo catalog, each task finds out which nodes it gets from the other tasks
			 * The nodes are located on grid 1 based on the distribution of the nodes on grid 0 */
			GlobalGrid[1].FindBufferNodes(GlobalGrid[0].locNodes);	

			/* Now exchange the halos in the requested buffer zones among the different tasks. */
			CommTasks.BufferSendRecv();

			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;
	
			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "Buffer exchanged in " << elapsed << "s. " << endl;
			}
#endif
	
			if (locTask == 0)
				cout << "Finding halo progentors, forwards..." << flush;

			iniTime = clock();
		
			/* Forward halo connections. This function also allocates the MergerTrees */
			FindProgenitors(0, 1);
			MPI_Barrier(MPI_COMM_WORLD);

			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;

			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "done in " << elapsed << "s. " << endl;
			}

			if (locTask == 0)
				cout << "Finding halo progentors, backwards..." << flush;

			iniTime = clock();
	
			/* Backward halo connections */
			FindProgenitors(1, 0);
			MPI_Barrier(MPI_COMM_WORLD);
	
			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;
	
			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "done in " << elapsed << "s. " << endl;
			}

			iniTime = clock();
#ifdef GATHER_TREES
#ifdef ZOOM
			if (locTask == 0)
			{
				cout << "GATHER_TREES only works without the ZOOM directive. Please re-compile without choosing ZOOM or GATHER_TREES."
				exit(0);
			}
#endif
			/* Gather all merger trees on the master task - this ensures buffer halos are correctly accounted for */
			CommTasks.GatherMergerTrees(0);
			CommTasks.GatherMergerTrees(1);
	
			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;
	
			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "Merger Trees gathered in " << elapsed << "s. " << endl;
			}

			iniTime = clock();

			if (locTask == 0)
				CleanTrees(iNumCat);
			
			/* Orphans will be found on the master task only, so we need to assign them to their parent task for tracking in the next step. */
			CommTasks.SyncOrphanHalos();

			if (locTask == 0)
				SettingsIO.WriteTree(iNumCat); 
#else
#ifndef ZOOM		/* If not gathering the trees, each task holds its part of the halo catalog, which needs to be synchronized on the buffer.
			 * FIXME: This is leaking somewhere and the sync is not working correctly. 
			 * Better always use the GATHER_TREES option instead */
			CommTasks.SyncMergerTreeBuffer();

			MPI_Barrier(MPI_COMM_WORLD);
	
			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;
	
			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "Merger Tree buffer synchronized in " << elapsed << "s. " << endl;
			}
#endif
			/* These operations are common for ZOOM and non-gather-trees mode */
			iniTime = clock();
			CleanTrees(iNumCat);

			SettingsIO.WriteTree(iNumCat); 	
			MPI_Barrier(MPI_COMM_WORLD);
#endif	/* GATHER_TREES */

#ifndef ZOOM		/* There are no halos on the buffer in ZOOM mode, so we skip this if ZOOM is defined. */
			CommTasks.CleanBuffer();
#endif
			ShiftHalosPartsGrids();

			endTime = clock();
			elapsed = double(endTime - iniTime) / CLOCKS_PER_SEC;

			if (locTask == 0)
			{
				SettingsIO.WriteLog(iNumCat, elapsed);
				cout << "Memory cleared and Merger Trees written in " << elapsed << "s. " << endl;
			}

			/* This cleans all the locMTree and locCleanTrees */
			FreeMergerTrees(iNumCat);

#ifdef VERBOSE
			/* Dump some information on the memory allocated on the various structures */
			MemoryCheck(iNumCat);
#endif

		}	/* Finish: the trees have now been built for this step */

		if (locTask == 0)
			cout << "The loop on halo and particle catalogs has finished." << endl;
	
		/* Sending the signal to close the log file */
		SettingsIO.WriteLog(-1, 0.0);
		try {
			CleanMemory(0);
		} catch (const std::exception&) {
			cout << "Possible error while cleaning memory." << endl;
		}
	}	

	MPI_Finalize();
	
	if (locTask == 0)	
		cout << "MPI finalized, memory cleaned. Exiting the program." << endl;

	exit(0);
}



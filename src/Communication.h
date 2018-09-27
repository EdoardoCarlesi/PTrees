#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <mpi.h>


class Communication {

public:
	// Empty constructor and destructor
	Communication() { };
	~Communication() { };	

#ifndef ZOOM
	void BroadcastAndGatherGrid(void);
#endif
	void BufferSendRecv(void);	

	/* Communicate orphan halo properties among different tasks */
	void SyncOrphanHalos(void);

private:
	// Determine the send and recv tasks consistently
	vector<int> sendTasks;
	vector<int> recvTasks;

#ifndef ZOOM
	void SetSendRecvTasks(void);

	// Communicate the buffers across all tasks
	void ExchangeBuffers(void);

	// Buffers for halo and node communication
	vector<vector<int>> buffIndexNodeHalo;
	
	// Here we store the full list of halo indexes
	vector<vector<int>> buffIndexSendHalo;
#endif
};
#endif

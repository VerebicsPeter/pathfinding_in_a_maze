__kernel void expand_wave_naive(
    int W, int H,
    __global const int *cost,
    __global uchar *wf_prev,
    __global uchar *wf_next,
    __global int *dist,
    int targetIdx,
    __global uchar *foundFlag   // OpenCL doesn’t allow pointers to host bool
) {
    int idx = get_global_id(0);
    if (idx >= W*H)
        return;
    
    // Return if not in wavefront
    if (wf_prev[idx] != 1)
        return;

    if (idx == targetIdx) {
        *foundFlag = 1;
        return;
    }

    // Get the distance set in the previous expansion
    int dcurr = dist[idx];
    int x = idx % W;
    int y = idx / W;

    // Check and add neighbors to next wavefront
    for (int k = 0; k < 4; k++) {
        int nx = x + ((k==0)?-1: (k==1)?1:0);
        int ny = y + ((k==2)?-1: (k==3)?1:0);
        
        if (nx < 0 || nx >= W || ny < 0 || ny >= H)
            continue;
        
        int j = ny*W + nx;
        
        // Skip walls
        if (cost[j] < 0)
            continue;

        // Check and add neighbors to next wavefront
        int old = atomic_cmpxchg(&dist[j], -1, dcurr + 1);
        if (old == -1) {
            wf_next[j] = 1;
        }
    }
}

__kernel void expand_wave_idxs(
    int W, int H, int WF_SIZE,
    __global const int *cost,
    __global int *wf_prev_idxs,
    __global int *wf_next_idxs,
    __global int *dist,
    int targetIdx,
    __global uchar *foundFlag   // OpenCL doesn’t allow pointers to host bool
) {
    int gidx = get_global_id(0);
    if (gidx >= WF_SIZE)
        return;

    int idx = wf_prev_idxs[gidx];

    // Return if not in wavefront (-1)
    if (idx < 0 || idx >= W*H)
        return;


    if (idx == targetIdx) {
        *foundFlag = 1;
        return;
    }
    
    // Get the distance set in the previous expansion
    int dcurr = dist[idx];
    int x = idx % W;
    int y = idx / W;

    // Check and add neighbors to next wavefront
    for (int k = 0; k < 4; k++) {
        int nx = x + ((k==0)?-1: (k==1)?1:0);
        int ny = y + ((k==2)?-1: (k==3)?1:0);
        
        if (nx < 0 || nx >= W || ny < 0 || ny >= H)
            continue;
        
        int j = ny*W + nx;
        
        // Skip walls
        if (cost[j] < 0)
            continue;

        // Check and add neighbors to next wavefront
        int old = atomic_cmpxchg(&dist[j], -1, dcurr + 1);
        if (old == -1) {
            wf_next_idxs[4*gidx+k] = j;
        }
    }
}

kernel void bodyInteraction(__global float *pos, __global float *vel, __global float3 *buff, const int bodies, const float step) {
  int gindex = get_global_id(0);
  int interactions=bodies*(bodies-1)/2;
  if (gindex < interactions) {
    // indexes
    int i,j,ij,ji;
    i = (int)floor(sqrt(0.25+2*gindex)+0.5);
    j = gindex-i*(i+1)/2;
    ij=i+j*(bodies);
    ji=j+i*(bodies);
    buff[ij]=(float3)(0.0f,0.0f,0.0f);
    buff[ji]=(float3)(0.0f,0.0f,0.0f);
    float3 pi,pj,vi,dp;
    float g=1000.0f;
    // getting i position
    pi.x = pos[i*6];
    pi.y = pos[i*6+1];
    pi.z = pos[i*6+2];

    // getting j position
    pj.x = pos[j*6];
    pj.y = pos[j*6+1];
    pj.z = pos[j*6+2];
    float dist, vec;

    dp = pi-pj;
    dist = sqrt(dot(dp,dp));

    if (dist > 0) { // Evitar la división por cero
        vec = g / (dist * dist * dist);
        buff[ij] =  vec*dp*step;
        buff[ji] = -vec*dp*step;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);
    if(gindex<bodies){
        float3 nv = (float3)(0.0f, 0.0f, 0.0f);
        for(int t=0;t<bodies;t++){
            nv+=buff[t+(gindex)*(bodies)];
        }
        float3 np=(float3)(pos[gindex*6], pos[gindex*6+1], pos[gindex*6+2])+nv*step;
        vel[gindex*3]=nv.x;
        vel[gindex*3+1]=nv.y;
        vel[gindex*3+2]=nv.z;

        pos[gindex*6]=np.x;
        pos[gindex*6+1]=np.y;
        pos[gindex*6+2]=np.z;
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
}

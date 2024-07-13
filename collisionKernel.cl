__kernel void bodyInteraction(__global float *pos, __global float *vel, const int bodies, const float step, const float radius) {
  int gindex = get_global_id(0);
  float G = 1000.0f;

  if (gindex < bodies) {
    float3 p, v;

    // Obtener la posición de este hilo
    p.x = pos[gindex * 6];
    p.y = pos[gindex * 6 + 1];
    p.z = pos[gindex * 6 + 2];

    // Obtener la velocidad de este hilo
    v.x = vel[gindex * 3];
    v.y = vel[gindex * 3 + 1];
    v.z = vel[gindex * 3 + 2];

    // Inicializar la aceleración
    float3 acc = (float3)(0.0f, 0.0f, 0.0f);
    float3 dp, dv;
    float dist, dist2, invDist2, force;

    // Variables para el manejo de colisiones
    float3 collision_impulse = (float3)(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < bodies; i++) {
      if (i != gindex) { 
        dp.x = pos[i * 6] - p.x;
        dp.y = pos[i * 6 + 1] - p.y;
        dp.z = pos[i * 6 + 2] - p.z;

        dist2 = dot(dp, dp);
        if (dist2 > 0) {
          dist = sqrt(dist2);

          // Verificar colisión
          if (dist < 2.0f * radius) {
            // Calcular velocidad relativa
            dv.x = vel[i * 3] - v.x;
            dv.y = vel[i * 3 + 1] - v.y;
            dv.z = vel[i * 3 + 2] - v.z;

            // Normalizar el vector de distancia
            float3 norm_dp = dp / dist;

            // Calcular la velocidad relativa en la dirección de la normal
            float relative_velocity = dot(dv, norm_dp);

            // Resolver la colisión
            if (relative_velocity < 0) { // Solo considerar colisiones si se están acercando
                collision_impulse += norm_dp * relative_velocity;
            }
          } else {
            // Calcular la fuerza gravitacional
            invDist2 = 1.0f / (dist * dist);
            force = G * invDist2;

            acc.x += force * dp.x;
            acc.y += force * dp.y;
            acc.z += force * dp.z;
          }
        }
      }
    }
    // Actualizar la velocidad con la fuerza acumulada
    v += acc * step;
    // Actualizar la velocidad con el impulso de colisión
    v += collision_impulse;
    // Actualizar la posición de la partícula
    p += v*step;

    barrier(CLK_GLOBAL_MEM_FENCE);

    pos[gindex*6] = p.x;
    pos[gindex*6+1] = p.y;
    pos[gindex*6+2] = p.z;

    vel[gindex*3] = v.x;
    vel[gindex*3+1] = v.y;
    vel[gindex*3+2] = v.z;

    barrier(CLK_GLOBAL_MEM_FENCE);
  }
}
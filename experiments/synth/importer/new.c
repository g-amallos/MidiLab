// Generated automatically
static float waveSampleInstrument(double phase) {
	double freqs[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	double amps[20] = {0.161009, 0.18194, 0.151337, 0.189113, 0.00986168, 0.0359957, 0.00530148, 0.169444, 0.00935529, 0.0117286, 0.00191257, 0.0308578, 0.00129378, 0.0069754, 0.00115615, 0.0145524, 0.000827077, 0.00449218, 0.000735, 0.0121131};
	double ret = 0.0;
	for (int i=0; i<20; i++) ret+=amps[i]*sin(phase*freqs[i]);
	return ret;
}
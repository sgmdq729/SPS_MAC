#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <numeric>
#include <unordered_map>
#include <algorithm>
#include <set>
#include "Vehicle.h"
#include "Logger.h"
//#define CON_COL
//#define LOG
#define PROPOSAL

#ifdef LOG
Logger logger;
#endif



using namespace std;

int subframe = 0;				//1ms�P�ʂ̃V�~�����[�V��������
constexpr int sensingWindow = 100;		//������Ԃő��M�^�C�~���O�����肷��͈�
constexpr int txSubCHLength = 1;			//���M�Ɏg�p����T�u�`���l����(�S�ԗ�����)
constexpr int numSubCH = 2;				//���T�u�`���l����

struct HashPair {
	template <class T1, class T2>
	size_t operator()(const pair < T1, T2>& p)const {
		auto hash1 = hash<T1>{}(p.first);
		auto hash2 = hash<T2>{}(p.second);

		size_t seed = 0;
		seed ^= hash1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

int main() {
	constexpr int warmupTime = 1500;			//����񂵎���
	constexpr int simTime = (100 * 100) + warmupTime;			//�S�̂̃V�~�����[�V��������(ms)

	constexpr int minVehicle = 50;			//�ŏ��ԗ���
	constexpr int maxVehicle = 200;		//�ő�ԗ���
	constexpr int numTrial = 100;					//�e�ԗ����ł̎��s��

	constexpr int T1 = 1;						//�I���E�B���h�ET1
	constexpr int T2 = 100;					//�I���E�B���h�ET2

	int timeGap = 0;				//nextEvent�܂ł̍�
	int nextEventSubframe;			//���̃C�x���g�̃V�~�����[�V��������
	vector<Vehicle*> vehicleList;			//�ԗ����i�[����vector
	vector<vector<int>> sensingList(sensingWindow + 1, vector<int>(numSubCH, 0));		//�X�V�������M���\�[�X


	//���s���Ԋ֌W�̐ݒ�
	chrono::system_clock::time_point mstart, mend;
	mstart = chrono::system_clock::now();
#ifdef PROPOSAL
	ofstream output("p0_p.csv");
#else
	ofstream output("p0.csv");
#endif // PROPOSAL



	//�ԗ����̃��[�v
	for (int numVehicle = minVehicle; numVehicle <= maxVehicle; numVehicle += 25) {
		cout << numVehicle << endl;
		int sendCounter = 0;
		int collisionCounter = 0;
		map<int, int> resultContinuous;
		map<int, int> resultColVehicles;
		int initCollisionCounter = 0;
		int reselectionAndCollisionCounter = 0;
		int noReselectionAndCollisionCounter = 0;


		//���s�񐔕����[�v
		for (int trial = 0; trial < numTrial; trial++) {
			//subframe������
			subframe = 0;
			unordered_map<pair<int, int>, int, HashPair> collisionPairs;
			bool warmupFlag = false;

			//�ԗ��C���X�^���X�̐���
			for (int i = 0; i < numVehicle; i++) {
				vehicleList.emplace_back(new Vehicle(i, numSubCH, txSubCHLength));
			}

			//subframe�̃��[�v�C���߂̑��M�\��subframe�܂Ői�߂�
			while (subframe < simTime) {
#ifdef LOG
				logger.writeSubframe(subframe);
#endif
				set<int> RCset;

				//subCH���ɑ��M���s���ԗ���id���i�[
				vector<Vehicle*> txVehicles;
				vector<vector<Vehicle*>> txVehiclesPerSubCH(numSubCH, vector<Vehicle*>());


				if (subframe != 0) {

					//sensingList�X�V
					sensingList.assign(sensingList.begin() + timeGap, sensingList.end());
					int gapSize = sensingWindow - sensingList.size() + 1;
					sensingList.insert(sensingList.end(), gapSize, vector<int>(numSubCH, 0));

				}

				//���M���s���ԗ��̓���
				for (Vehicle*& vehicle : vehicleList) {
					//���M���s���ԗ��̔���
					if (subframe == vehicle->getTxResourceLocation().first) {
						RCset.insert(vehicle->getRC());
						txVehicles.emplace_back(vehicle);
						txVehiclesPerSubCH[vehicle->getTxResourceLocation().second].emplace_back(vehicle);
						if (warmupTime < subframe)
							sendCounter++;

						//���M�Ɏg�p�����T�u�`���l�����}�[�N
							//sensingList�̍Ō������������subframe�Ȃ̂�sensingWindow�ԖڂɋL�^
						sensingList[sensingWindow][vehicle->getTxResourceLocation().second] = 1;

						//RC�̌��Z
						vehicle->decrementRC();
#ifdef LOG
						logger.writeSend(vehicle->getId(), vehicle->getTxResourceLocation(), vehicle->getRC());
#endif
					}
				}

				if (!warmupFlag && warmupTime <= subframe) {
					warmupFlag = true;
					for (Vehicle* vehicle : vehicleList) {
						vehicle->resetRC();
					}
				}
#ifdef CON_COL
				if (warmupTime <= subframe) {
					for (vector<int> vec : txIdPerSubCH) {
						if (vec.size() > 0) {
							auto itr = collisionPairs.begin();
							if (vec.size() == 1) {
								while (itr != collisionPairs.end()) {
									if (itr->first.first == vec[0] || itr->first.second == vec[0]) {
										if (itr->second < 5) {
											exit(-2);
										}
										resultContinuous[itr->second]++;
										collisionPairs.erase(itr++);
									}
									else ++itr;
								}
							}
							else {
								resultColVehicles[vec.size()]++;
								while (itr != collisionPairs.end()) {
									if ((find(vec.begin(), vec.end(), itr->first.first) == vec.end() && find(vec.begin(), vec.end(), itr->first.second) != vec.end())
										|| (find(vec.begin(), vec.end(), itr->first.first) != vec.end() && find(vec.begin(), vec.end(), itr->first.second) == vec.end())) {
										if (itr->second < 5) {
											exit(-2);
										}
										resultContinuous[itr->second]++;
										collisionPairs.erase(itr++);
									}
									else ++itr;
								}

								sort(vec.begin(), vec.end());
								for (auto itr1 = vec.begin(); itr1 != vec.end(); ++itr1) {
									for (auto itr2 = vec.end() - 1; itr2 != itr1; --itr2) {
										collisionPairs[make_pair(*itr1, *itr2)]++;
									}
								}
							}
						}
					}
				}
#endif

				for (auto&& vec : txVehiclesPerSubCH) {
					if (warmupTime <= subframe && vec.size() > 1) {
						collisionCounter += vec.size();
#ifdef LOG
						logger.writeCollision(vec[0]->getTxResourceLocation().second);
#endif 
					}
					for (auto&& vehicle : vec) {
						//RC��0�ɂȂ�����đI��
						if (vehicle->getRC() <= 0) {
							//�đI���Ŏg�p����selectionList��ݒ�
							//���\�[�X�̍đI�𔻒�
							vehicle->selectResource(sensingList, subframe);
						}
						//RC��0�o�Ȃ��ꍇ�͑��M�X�P�W���[����RRI���X�V
						else
							vehicle->txResourceUpdate();
					}
				}
#ifdef CON_COL
				for (auto&& vec : txVehiclesPerSubCH) {
					for (auto&& vehicle : vec) {
						if (vehicle->wasRCComplete)
							initCollisionCounter++;
					}
				}


				for (auto&& elem : collisionPairs) {
					for (auto&& vec : txIdPerSubCH) {
						if (find(vec.begin(), vec.end(), elem.first.first) != vec.end() && find(vec.begin(), vec.end(), elem.first.second) != vec.end()) {
							auto target1 = vehicleList[*find(vec.begin(), vec.end(), elem.first.first)];
							auto target2 = vehicleList[*find(vec.begin(), vec.end(), elem.first.second)];
							if (target1->isReselection && target2->isReselection) {
								if (target1->getTxResourceLocation() == target2->getTxResourceLocation()) {
									reselectionAndCollisionCounter++;
								}
							}
							else if (target1->isNotReselection)
								noReselectionAndCollisionCounter++;
							else if (target2->isNotReselection)
								noReselectionAndCollisionCounter++;
						}
					}
				}
#endif CON_COL

				//���C�x���g���Ԃ̐ݒ�
				nextEventSubframe = INT_MAX;	//���̏����l
				//���߂̃C�x���g���Ԃ�������
				for (Vehicle* vehicle : vehicleList) {
#ifdef PROPOSAL
					if (find(txVehicles.begin(), txVehicles.end(), vehicle) == txVehicles.end()) {
						if (find(RCset.begin(), RCset.end(), vehicle->getRC() + 1) != RCset.end()) {
							vehicle->changeRC();
						}
					}
#endif
					auto comparison = vehicle->getTxResourceLocation().first;
					nextEventSubframe = nextEventSubframe > comparison ? comparison : nextEventSubframe;
				}
				//���݂̎��ԂƎ��̃C�x���g�܂ł̎��ԍ�
				timeGap = nextEventSubframe - subframe;

				//�����X�V
				subframe = nextEventSubframe;
#ifdef LOG
				logger.writeSubframeTerm();
#endif
			}

			//�ԗ��C���X�^���X�̍폜
			for (Vehicle*& vehicle : vehicleList) {
				delete vehicle;
			}
#ifdef LOG
			logger.writeTerm();
#endif
			vector<Vehicle*>().swap(vehicleList);
			collisionPairs.clear();
		}

		output << numVehicle << "," << (double)collisionCounter / (double)sendCounter << endl;
		cout << (double)collisionCounter / (double)sendCounter << endl;
	}
	output.close();
	mend = chrono::system_clock::now();
	std::cout << endl << "���s���� " << chrono::duration_cast<chrono::minutes>(mend - mstart).count() << " m" << endl;
	return 0;
}
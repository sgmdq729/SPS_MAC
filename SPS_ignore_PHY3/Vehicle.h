#pragma once
#include <vector>
#include <random>
#include <iostream>
#include <utility>
#include <map>
#include "Logger.h"

using namespace std;

constexpr int RRI = 100;		//���M����
constexpr double probResourceKeep = 0.0;				//���\�[�X�L�[�v�m��

extern Logger logger;

class Vehicle {
private:
	int id;				//�ԗ���id
	int subframe = 0;	//����(ms)
	int numSubCH;		//���T�u�`���l����
	int txSubCHLength;	//���M�Ɏg�p����T�u�`���l����
	int RC = -1;		//���\�[�X���Z���N�V�����J�E���^(���̏����l)
	int C1, C2;			//RC�͈̔�
	int txTimingRange;	//������Ԃő��M�^�C�~���O�����肷��͈�

	int setRC = -1;
	int previousRC = -1;
	int prePreRC = -1;
	pair<int, int> previousLocation;
	pair<int, int> prePreLocation;
	pair<int, int> txResourceLocation;	//���ɑ��M����subframe��subCH
	vector<pair<int, int>> SB;
	random_device seed;
	mt19937 engine;
	uniform_int_distribution<> distRC, distTXTiming, distTXsubCH, distSB; //RC�̑I���C���M�^�C�~���O�̑I��,���M�T�u�`���l���̑I��
	uniform_real_distribution<> distResourceKeep;		//���\�[�X���Z���N�V�����̔���p


public:
	bool isReselection = false;
	bool isNotReselection = false;
	bool wasRCComplete = false;
	int getId() {
		return id;
	}

	int getRC() {
		return RC;
	}

	void decrementRC() {
		RC--;
	}

	int getSubframe() {
		return subframe;
	}


	pair<int, int> getTxResourceLocation() {
		return txResourceLocation;
	}

	void printResourceRocation() {
		cout << "subCHIndex " << txResourceLocation.first << ", subCHLength " << txResourceLocation.second << endl;
	}

	//�Z���V���O�x�[�XSPS
	void selectResource(vector<vector<int>>& candidateResource, int subframe) {
		wasRCComplete = true;

		prePreRC = previousRC;
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;

		prePreLocation = previousLocation;
		previousLocation = txResourceLocation;

		//probResouceKeep�̔���
		if (distResourceKeep(engine) > probResourceKeep) {
			isReselection = true;
			for (auto itr_subFrame = candidateResource.begin() + 1; itr_subFrame != candidateResource.end() - 1; itr_subFrame++) {
				//itr_subCH_init:���g�p�T�u�`���l�������̎n�_
				for (auto itr_subCH = itr_subFrame->begin(); itr_subCH != itr_subFrame->end(); itr_subCH++) {
					if (*itr_subCH == 0) {
						//�T�u�`���l�����󂢂Ă����ꍇ�C
						SB.emplace_back(make_pair(distance(candidateResource.begin(), itr_subFrame), distance(itr_subFrame->begin(), itr_subCH)));
					}
				}
			}

			//SB���烉���_���Ɉ���M���\�[�X��I��
			uniform_int_distribution<>::param_type paramSB(0, SB.size() - 1);
			distSB.param(paramSB);
			txResourceLocation = SB[distSB(engine)];
			txResourceLocation.first += subframe;
			SB.clear();
#ifdef LOG
			logger.writeReselection(id, txResourceLocation, RC);
#endif
		}
		else {
			isNotReselection = true;
			txResourceLocation.first += RRI;
#ifdef LOG
			logger.writeKeep(id, txResourceLocation, RC);
#endif
		}
	}

	//RC=0�ƂȂ�Ȃ��ꍇ
	void txResourceUpdate() {
		isReselection = false;
		isNotReselection = false;
		wasRCComplete = false;
		txResourceLocation.first += RRI;
	}

	void changeRC() {
		uniform_int_distribution<> dist(-5, 5);
		int tmp = RC;
		previousRC = setRC;
		RC += dist(engine);
		setRC = RC;
#ifdef LOG
		logger.writeRCChange(id, RC, tmp);
#endif
	}

	void resetRC() {
		previousRC = setRC;
		RC = distRC(engine);
		setRC = RC;
	}

	//�R���X�g���N�^
	Vehicle(int id, int subCH, int txSubCH) {
		//�v���C�x�[�g�ϐ��ɑ��
		this->id = id;
		numSubCH = subCH;
		txSubCHLength = txSubCH;
		txTimingRange = 100;

		//RC�ݒ�͈͂��`
		if (RRI >= 100) {
			C1 = 5;
			C2 = 15;
		}
		else if (RRI == 50) {
			C1 = 10;
			C2 = 30;
		}
		else if (RRI == 20) {
			C1 = 25;
			C2 = 75;
		}
		else {
			cout << "RRI error" << endl;
			exit(1);
		}

		engine.seed(seed());
		//engine.seed(id);


		//RC�̌���
		uniform_int_distribution<>::param_type paramRC(C1, C2);
		distRC.param(paramRC);
		RC = distRC(engine);
		setRC = RC;

		//���M���\�[�X�̌���
		uniform_int_distribution<>::param_type paramTXTiming(0, txTimingRange - 1);				//���Ԏ������̐ݒ�
		uniform_int_distribution<>::param_type paramTXSubCH(0, numSubCH - txSubCHLength);		//���g�������̐ݒ�
		uniform_real_distribution<>::param_type paramResourceKeep(0, 1);						//RC�̏����l

		distTXTiming.param(paramTXTiming);
		distTXsubCH.param(paramTXSubCH);
		distResourceKeep.param(paramResourceKeep);

		//������ԂŃ����_���ɑ��M����T�u�t���[���C�T�u�`���l��������
		txResourceLocation.first = distTXTiming(engine);
		txResourceLocation.second = distTXsubCH(engine);
	}
};
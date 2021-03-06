// 11. Быстрая сортировка с простым слиянием.

#include "pch.h"
#include <iostream>
#include <mpi.h>
#include <ctime>
#include <string>
using namespace std;

double* genArr(const int size) {

	double*  arr = new double[size];
	for (int i = 0; i < size; i++)
		arr[i] = rand() % 100 - 50;


	return  arr;
}

void printArr(double* arr, int size) {

	if (size < 20) {
		for (int i = 0; i < size; i++) {
			cout << arr[i];
		}
	}
	return;
}

void quickSort(double* arr, int l, int r) {

	double m = arr[(r + l) / 2];
	double temp = 0;

	int i = l, j = r;

	while (i <= j) {
		while (arr[i] < m)
			i++;
		while (arr[j] > m)
			j--;
		if (i <= j) {
			if (i < j) {
				temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
			i++;
			j--;
		}
	}
	if (j > l)
		quickSort(arr, l, j);
	if (r > i)
		quickSort(arr, i, r);
}

void Merge(double* arr, int size, int l1, int r1, int l2, int r2) {

	double* tempArr = new double[size];

	int i = l1, j = l2, k = l1;

	while ((i <= r1) && (j <= r2)) {			//пока не дойдем до конца одного из массивов  

		if (arr[i] < arr[j]) {					//перепишем по порядку все элементы первого массива, 
			tempArr[k++] = arr[i++];				//которые меньше текущего элемента второго массива
		}
		else {									//перепишем текущий элемент второго массива
			tempArr[k++] = arr[j++];
		}
	}
	if (i > r1)                             //если дошли до конца первого массива - перепишем второй
		for (j; j <= r2; j++) {
			tempArr[k++] = arr[j];
		}
	else										 //если дошли до конца второго массива - перепишем первый
		for (i; i <= r1; i++) {
			tempArr[k++] = arr[i];
		}

	for (i = l1; i <= r2; i++)
		arr[i] = tempArr[i];
}

int main(int argc, char* argv[]) {

	double timeStartLine = 0, timeStartParal = 0;
	double timeEndLine = 0, timeEndParal = 0;

	int errCode;
	errCode = MPI_Init(&argc, &argv);
	if (errCode != 0) {
		return errCode;
	}

	int procNum, procRank;
	int *sendNum = NULL;	 //кол-во элементов в блоке
	int *displ = NULL;		 //смещение блока относительно начала

	double* arr = NULL;
	int size = 10;

	MPI_Comm_size(MPI_COMM_WORLD, &procNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	sendNum = new int[procNum];
	int step = size / procNum;


	if (procRank == 0) {

		arr = genArr(size);

		if (size < 20) {
			string st = "";
			st += "arr =  ";
			for (int i = 0; i < size; i++)
			{
				st += to_string(arr[i]) + " ";
			}
			st += "\n";
			cout << st;
		}

		displ = new int[procNum];

		int balance = size % procNum;
		int temp = 0;

		for (int i = 0; i < procNum; i++) {
			displ[i] = temp;
			sendNum[i] = step;
			if (balance != 0) {
				sendNum[i]++;
				balance--;
			}
			temp += sendNum[i];
		}
	}

	MPI_Bcast(sendNum, procNum, MPI_INT, 0, MPI_COMM_WORLD);
	double *buf = new double[sendNum[procRank]];

	MPI_Scatterv(arr, sendNum, displ, MPI_DOUBLE, buf, sendNum[procRank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

	if (procRank == 0)
		timeStartParal = MPI_Wtime();

	quickSort(buf, 0, sendNum[procRank] - 1);

	/*string st = "";
	st += "procRank = " + to_string(procRank) + " ";
	for (int i = 0; i < sendNum[procRank]; i++)
	{
		st += to_string(buf[i]) + " ";
	}
	st += "\n";
	cout << st;*/

	int s = procNum, m = 1;

	while (s > 1) {

		s = s / 2 + s % 2;

		if ((procRank - m) % (2 * m) == 0) {

			//	cout << "1";
			//MPI_Send(&sendNum[procRank], 1, MPI_INT, 0, 8080, MPI_COMM_WORLD);

			MPI_Send(&sendNum[procRank], 1, MPI_INT, procRank - m, 1010, MPI_COMM_WORLD);
			MPI_Send(buf, sendNum[procRank], MPI_DOUBLE, procRank - m, 2020, MPI_COMM_WORLD);
		}

		if ((procRank % (2 * m) == 0) && (procNum - procRank > m)) {

			MPI_Status status;
			int k1;

			//cout << "0" << " ";
			//MPI_Recv(&k1, 1, MPI_INT, 1, 8080, MPI_COMM_WORLD, &status);

			MPI_Recv(&k1, 1, MPI_INT, procRank + m, 1010, MPI_COMM_WORLD, &status);

			double* y = new double[(sendNum[procRank] + k1)];
			MPI_Recv(y, k1, MPI_DOUBLE, procRank + m, 2020, MPI_COMM_WORLD, &status);

			for (int i = 0; i < sendNum[procRank]; i++)
				y[i + k1] = buf[i];

			Merge(y, sendNum[procRank] + k1, 0, k1 - 1, k1, sendNum[procRank] + k1 - 1);

			buf = new double[sendNum[procRank] + k1];
			for (int i = 0; i < sendNum[procRank] + k1; i++)
				buf[i] = y[i];

			sendNum[procRank] = sendNum[procRank] + k1;

		}
		m = 2 * m;
	}

	if (procRank == 0) {

		timeEndParal = MPI_Wtime();

		for (int i = 0; i < size; i++)
		{
			arr[i] = buf[i];
		}

		if (size < 20) {
			string st = "";
			st += "paralSortArr =  ";
			for (int i = 0; i < size; i++)
			{
				st += to_string(arr[i]) + " ";
			}
			st += "\n";
			cout << st;
		}

		cout << "parallTime = " << timeEndParal - timeStartParal << endl;
	}

	MPI_Finalize();

	return 0;
}



//"C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" -n 4 lab3MPI.exe
// слияние гиперкубом 
// 11. Быстрая сортировка с простым слиянием.

#include "pch.h"
#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <ctime>
#include <string>
using namespace std;

// для qsort()
int compare(const void * x1, const void * x2)   // функция сравнения элементов массива
{
	return (*(double*)x1 - *(double*)x2);              // если результат вычитания равен 0, то числа равны, < 0: x1 < x2; > 0: x1 > x2
}

bool isEqual(double *v1, double *v2, int size) {
	for (int i = 0; i < size; i++)
		if (v1[i] != v2[i])
			return false;
	return true;
}

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
	double* arrForLinear = NULL;	//копия сгенерированного массива
	double* arrForCheck = NULL;
	int size = 100000;

	MPI_Comm_size(MPI_COMM_WORLD, &procNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	if (procRank == 0) {

		cout << "Input the size" << endl;
		cin >> size;
	}

	sendNum = new int[procNum];
	int step = size / procNum;

	if (procRank == 0) {

		arr = genArr(size);

		arrForLinear = new double[size];
		arrForCheck = new double[size];
		for (int i = 0; i < size; i++) {
			arrForLinear[i] = arr[i];
			arrForCheck[i] = arr[i];
		}

		qsort(arrForCheck, size, sizeof(double), compare);

		//_____________________последовательная версия алгоритма________________________________________________________________________
		timeStartLine = MPI_Wtime();
		quickSort(arrForLinear, 0, size/2-1);
		quickSort(arrForLinear, size / 2, size - 1);
		Merge(arrForLinear,size,0,size/2-1, size / 2, size - 1);
		timeEndLine = MPI_Wtime();
		//______________________________________________________________________________________________________________________________

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

	if (procRank == 0)
		timeStartParal = MPI_Wtime();

	MPI_Bcast(sendNum, procNum, MPI_INT, 0, MPI_COMM_WORLD);
	double *buf = new double[sendNum[procRank]];

	MPI_Scatterv(arr, sendNum, displ, MPI_DOUBLE, buf, sendNum[procRank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

	quickSort(buf, 0, sendNum[procRank] - 1);

	int s = procNum, m = 1;

	while (s > 1) {

		s = s / 2 + s % 2;

		if ((procRank - m) % (2 * m) == 0) {

			MPI_Send(&sendNum[procRank], 1, MPI_INT, procRank - m, 1010, MPI_COMM_WORLD);
			MPI_Send(buf, sendNum[procRank], MPI_DOUBLE, procRank - m, 2020, MPI_COMM_WORLD);
		}

		if ((procRank % (2 * m) == 0) && (procNum - procRank > m)) {

			MPI_Status status;
			int k1;

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
		
		for (int i = 0; i < size; i++)
		{
			arr[i] = buf[i];
		}
		timeEndParal = MPI_Wtime();
		

		if (size < 20) {
			string st = "";
			st += "paralSortArr =  ";
			for (int i = 0; i < size; i++)
			{
				st += to_string(arr[i]) + " ";
			}
			st += "\n";
			cout << st;

			st = "";
			st += "lineSortArr =  ";
			for (int i = 0; i < size; i++)
			{
				st += to_string(arrForLinear[i]) + " ";
			}
			st += "\n";
			cout << st;
		}

		cout << endl;
		if(isEqual(arr, arrForCheck, size))
			cout << "all right"<<endl;
		else
			cout << "ERROR" << endl;

		cout << "parallTime = " << timeEndParal - timeStartParal << endl;
		cout << "linearTime = " << timeEndLine - timeStartLine << endl;
	}

	MPI_Finalize();

	return 0;
}

//"C:\Program Files\Microsoft MPI\Bin\mpiexec.exe" -n 4 lab3MPI.exe

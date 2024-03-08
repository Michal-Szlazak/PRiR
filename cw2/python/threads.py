
import sys
import threading
import time

class Matrix:
    def __init__(self, rows, cols):
        self.rows = rows
        self.cols = cols
        self.data = [[0] * cols for _ in range(rows)]
        self.lock = threading.Lock() 

    def set(self, row, col, value):
        with self.lock:
            self.data[row][col] = value

    def get(self, row, col):
        return self.data[row][col]

    def display(self):
        for row in self.data:
            print(row)


def read_matrix (filename):
    with open(filename, "r") as file:
        rows = int(file.readline())
        cols = int(file.readline())
        matrix = Matrix(rows, cols)
        for i in range(rows):
            line = file.readline().split()
            for j in range(cols):
                matrix.set(i, j, float(line[j]))
        return matrix 


def create_intervals(num_threads, max_index):
    intervals = []

    rest = max_index % num_threads
    part = max_index // num_threads
    addition = 0

    for i in range(num_threads):
        if rest > 0:
            addition += 1
            rest -= 1
            intervals.append((i * part + addition, i * part + part + addition))
        else:
            intervals.append((i * part + addition, i * part + part + addition))

    print(intervals)
    return intervals



def calculate_result(A, B, C, intervals, threadId):

    start_index, end_index = intervals

    for i in range(start_index, end_index):
        row = i // C.cols
        col = i % C.cols
        value = 0
        for j in range(A.cols):
            value += A.get(row, j) * B.get(j, col)
        C.set(row, col, value)


def calculate_forbenius_norm(C, intervals, threadId, result):

    result[threadId] = 0
    start_index, end_index = intervals

    for i in range(start_index, end_index):
        row = i // C.cols
        col = i % C.cols
        result[threadId] += C.get(row, col) ** 2


def main():

    if len(sys.argv) != 3:
        print("Usage: threads.py <number of threads> <mode>")
        sys.exit(1)
    
    num_threads = int(sys.argv[1])
    mode = sys.argv[2]

    filenameA = ""
    filenameB = ""

    if mode == "base":
        filenameA = "data/A.txt"
        filenameB = "data/B.txt"
    elif mode == "gen":
        filenameA = "data/genA.txt"
        filenameB = "data/genB.txt"
    else:
        print("Invalid mode")
        sys.exit(1)

    A = read_matrix(filenameA)
    B = read_matrix(filenameB)

    if A.cols != B.rows:
        print("Invalid matrix dimensions")
        sys.exit(1)
    
    C = Matrix(A.rows, B.cols)

    intervals = create_intervals(num_threads, C.rows * C.cols)

    begin = time.time()
    threads = []

    for i in range(num_threads):
        t = threading.Thread(target=calculate_result, args=(A, B, C, intervals[i], i))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()
    
    # C.display()

    forbenius = [0] * num_threads

    for i in range(num_threads):
        t = threading.Thread(target=calculate_forbenius_norm, args=(C, intervals[i], i, forbenius))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    forbenius_norm = 0

    for i in range(num_threads):
        forbenius_norm += forbenius[i]
    
    forbenius_norm = forbenius_norm ** 0.5

    end = time.time()

    print("Time: ", end - begin)

    print("Forbenius norm: ", forbenius_norm)

if __name__ == "__main__":
    main()
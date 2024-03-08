import java.io.File;
import java.io.IOException;
import java.util.Scanner;
import java.util.concurrent.atomic.DoubleAdder; 

public class Threads {

    public static void main(String[] args) {

        if(args.length != 2) {
            System.out.println("Not enough arguments");
            System.out.println("Usage: java Threads <number of threads> <number of iterations> <mode>");
            System.out.println("Mode: base, gen");
            System.exit(1);
        }

        int numThreads = Integer.parseInt(args[0]);
        String mode = args[1];

        String filenameA;
        String filenameB;

        if(mode.equals("base")) {
            filenameA = "data/A.txt";
            filenameB = "data/B.txt";
        } else if(mode.equals("gen")) {
            filenameA = "data/genA.txt";
            filenameB = "data/genB.txt";
        } else {
            System.out.println("Invalid mode");
            return;
        }

        Matrix A = null;
        Matrix B = null;
        Matrix C;

        try (Scanner scanner = new Scanner(new File(filenameA))) {
         
            int rows = scanner.nextInt();
            int cols = scanner.nextInt();
            A = new Matrix(rows, cols);

            for(int i = 0; i < rows; i++) {
                for(int j = 0; j < cols; j++) {
                    A.set(i, j, scanner.nextDouble());
                }
            }

        } catch (IOException e) {
            System.err.println("Error reading the file: " + e.getMessage());
        }


        try (Scanner scanner = new Scanner(new File(filenameB))) {
         
            int rows = scanner.nextInt();
            int cols = scanner.nextInt();
            B = new Matrix(rows, cols);

            for(int i = 0; i < rows; i++) {
                for(int j = 0; j < cols; j++) {
                    B.set(i, j, scanner.nextDouble());
                }
            }
            
        } catch (IOException e) {
            System.err.println("Error reading the file: " + e.getMessage());
        }

        C = new Matrix(A.getRows(), B.getCols());

        int[] intervals = calculateIntervals(numThreads, C.getRows() * C.getCols());

        Thread[] threads = new Thread[numThreads];

        long startTime = System.currentTimeMillis();

        for(int i = 0; i < numThreads; i++) {
            threads[i] = new Worker(A, B, C, intervals, i);
            threads[i].start();
        }

        for(int i = 0; i < numThreads; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                System.err.println("Error joining the threads: " + e.getMessage());
            }
        }

        // C.print();

        DoubleAdder sum = new DoubleAdder();

        for(int i = 0; i < numThreads; i++) {
            threads[i] = new ForbeniusWorker(C, intervals, sum, i);
            threads[i].start();
        }

        for(int i = 0; i < numThreads; i++) {
            try {
                threads[i].join();
            } catch (InterruptedException e) {
                System.err.println("Error joining the threads: " + e.getMessage());
            }
        }

        long endTime = System.currentTimeMillis();

        System.out.println("Forbenius norm: " + Math.sqrt(sum.doubleValue()));
        System.out.println("Time: " + (endTime - startTime) + "ms");
    }

    private static int[] calculateIntervals(int numOfThreads, int maxIndex) {

        int[] intervals = new int[numOfThreads * 2];

        int rest = maxIndex % numOfThreads;
        int part = (maxIndex - rest) / numOfThreads;
        int addition = 0;
        int index = 0;

        for(int i = 0; i < numOfThreads; i++) {
            if(rest > 0) {
                intervals[index] = i * part + addition;    
                intervals[++index] = i * part + part + addition + 1;
                addition++;
                rest--;
            } else {
                intervals[index] = i * part + addition;
                intervals[++index] = i * part + part + addition;
            }

            index++;
        }

        // for(int i = 0; i < intervals.length; i++) {
        //     System.out.println(intervals[i] + " - " + intervals[++i]);
        // }

        return intervals;
    }

    private static class Worker extends Thread {
        private Matrix A;
        private Matrix B;
        private Matrix C;
        private int[] intervals;
        private int threadId;

        public Worker(Matrix A, Matrix B, Matrix C, int[] intervals, int threadId) {
            this.A = A;
            this.B = B;
            this.C = C;
            this.intervals = intervals;
            this.threadId = threadId;
        }

        public void run() {
            for(int i = intervals[threadId * 2]; i < intervals[threadId * 2 + 1]; i++) {
                int row = i / C.getCols();
                int col = i % C.getCols();
                double sum = 0;

                for(int j = 0; j < A.getCols(); j++) {
                    sum += A.get(row, j) * B.get(j, col);
                }
                // System.out.println("Thread " + Thread.currentThread().getId() + " is working on row " + row + " and col " + col + " with sum " + sum);
                C.set(row, col, sum);
            }
        }
    }

    private static class ForbeniusWorker extends Thread{

        private Matrix C;
        private int[] intervals;
        private DoubleAdder sum;
        private int threadId;

        public ForbeniusWorker(Matrix C, int[] intervals, DoubleAdder sum, int threadId) {
            this.C = C;
            this.intervals = intervals;
            this.sum = sum;
            this.threadId = threadId;
        }

        public void run() {
            for(int i = intervals[threadId * 2]; i < intervals[threadId * 2 + 1]; i++) {
                int row = i / C.getCols();
                int col = i % C.getCols();
                sum.add(Math.abs(C.get(row, col) * C.get(row, col)));
            }
        }
    }
}

class Matrix {
    private double[][] matrix;
    private int rows;
    private int cols;

    public Matrix(int rows, int cols) {
        matrix = new double[rows][cols];
        this.rows = rows;
        this.cols = cols;
    }

    synchronized public void set(int row, int col, double value) {
        matrix[row][col] = value;
    }

    public double get(int row, int col) {
        return matrix[row][col];
    }

    public int getRows() {
        return rows;
    }

    public int getCols() {
        return cols;
    }

    public void print() {
        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < cols; j++) {
                System.out.print(matrix[i][j] + " ");
            }
            System.out.println();
        }
    }
}
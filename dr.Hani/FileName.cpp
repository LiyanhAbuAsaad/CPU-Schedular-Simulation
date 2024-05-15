#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>

using namespace std;

struct Process {
    int pid;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int startTime;
    int finishTime;
    int waitingTime;
    int turnaroundTime;

    Process(int id, int arrival, int burst)
        : pid(id), arrivalTime(arrival), burstTime(burst), remainingTime(burst), startTime(-1) {}
};

struct SRTComparator {
    bool operator()(const Process* a, const Process* b) {
        if (a->remainingTime == b->remainingTime) {
            return a->arrivalTime > b->arrivalTime;
        }
        return a->remainingTime > b->remainingTime;
    }
};

void FCFS(vector<Process>& processes, int contextSwitchTime);
void SRT(vector<Process>& processes, int contextSwitchTime);
void RR(vector<Process>& processes, int contextSwitchTime, int quantum);
void displayResults(const vector<Process>& processes, const string& algorithm);
void generateGanttChart(const vector<pair<int, string>>& timeline, const string& algorithm);

int main() {
    ifstream infile("input.txt");
    if (!infile) {
        cerr << "Error opening input file.\n";
        return 1;
    }

    int contextSwitchTime, quantum;
    infile >> contextSwitchTime >> quantum;

    vector<Process> processes;
    int pid, arrivalTime, burstTime;
    while (infile >> pid >> arrivalTime >> burstTime) {
        processes.emplace_back(pid, arrivalTime, burstTime);
    }

    infile.close();

    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
        });

    FCFS(processes, contextSwitchTime);
    SRT(processes, contextSwitchTime);
    RR(processes, contextSwitchTime, quantum);

    return 0;
}

void FCFS(vector<Process>& processes, int contextSwitchTime) {
    int currentTime = 0;
    vector<pair<int, string>> timeline;

    for (auto& process : processes) {
        if (currentTime < process.arrivalTime) {
            currentTime = process.arrivalTime;
        }
        process.startTime = currentTime;
        process.finishTime = currentTime + process.burstTime;
        process.turnaroundTime = process.finishTime - process.arrivalTime;
        process.waitingTime = process.startTime - process.arrivalTime;
        currentTime += process.burstTime + contextSwitchTime;

        timeline.push_back({ process.burstTime, "P" + to_string(process.pid) });
        if (contextSwitchTime > 0 && &process != &processes.back()) {
            timeline.push_back({ contextSwitchTime, "CS" });
        }
    }

    generateGanttChart(timeline, "FCFS");
    displayResults(processes, "FCFS");
}

void SRT(vector<Process>& processes, int contextSwitchTime) {
    auto comp = [](Process* a, Process* b) { return a->remainingTime > b->remainingTime || (a->remainingTime == b->remainingTime && a->arrivalTime > b->arrivalTime); };
    priority_queue<Process*, vector<Process*>, decltype(comp)> pq(comp);

    int currentTime = 0, idx = 0;
    vector<Process*> activeProcesses;
    vector<pair<int, string>> timeline;

    while (idx < processes.size() || !pq.empty()) {
        while (idx < processes.size() && processes[idx].arrivalTime <= currentTime) {
            pq.push(&processes[idx]);
            if (processes[idx].startTime == -1) {
                processes[idx].startTime = currentTime;
            }
            idx++;
        }

        if (!pq.empty()) {
            Process* current = pq.top();
            pq.pop();
            int executionTime = 1; // SRT processes one unit at a time
            current->remainingTime -= executionTime;
            currentTime += executionTime;
            timeline.push_back({ executionTime, "P" + to_string(current->pid) });

            if (current->remainingTime > 0) {
                pq.push(current);
                if (!pq.empty()) {
                    timeline.push_back({ contextSwitchTime, "CS" });
                    currentTime += contextSwitchTime;
                }
            }
            else {
                current->finishTime = currentTime;
                current->turnaroundTime = current->finishTime - current->arrivalTime;
                current->waitingTime = current->turnaroundTime - current->burstTime;
            }
        }
        else {
            currentTime++;  // Idle time
        }
    }

    generateGanttChart(timeline, "SRT");
    displayResults(processes, "SRT");
}

void RR(vector<Process>& processes, int contextSwitchTime, int quantum) {
    queue<Process*> readyQueue;
    vector<pair<int, string>> timeline;

    int currentTime = 0, idx = 0;
    while (idx < processes.size() || !readyQueue.empty()) {
        while (idx < processes.size() && processes[idx].arrivalTime <= currentTime) {
            readyQueue.push(&processes[idx]);
            if (processes[idx].startTime == -1) {
                processes[idx].startTime = currentTime;
            }
            idx++;
        }

        if (!readyQueue.empty()) {
            Process* current = readyQueue.front();
            readyQueue.pop();
            int timeSlice = min(quantum, current->remainingTime);
            current->remainingTime -= timeSlice;
            currentTime += timeSlice;
            timeline.push_back({ timeSlice, "P" + to_string(current->pid) });

            if (current->remainingTime > 0) {
                readyQueue.push(current);
                timeline.push_back({ contextSwitchTime, "CS" });
                currentTime += contextSwitchTime;
            }
            else {
                current->finishTime = currentTime;
                current->turnaroundTime = current->finishTime - current->arrivalTime;
                current->waitingTime = current->turnaroundTime - current->burstTime;
            }
        }
        else {
            currentTime++;  // Idle time
        }
    }

    generateGanttChart(timeline, "RR");
    displayResults(processes, "RR");
}

void generateGanttChart(const vector<pair<int, string>>& timeline, const string& algorithm) {
    cout << "Gantt Chart for " << algorithm << ":\n";
    cout << "-------------------------------------------\n";
    int totalTime = 0;
    for (const auto& event : timeline) {
        cout << "| " << event.second << " ";
        totalTime += event.first;
    }
    cout << "|\n";
    cout << "-------------------------------------------\n";
    cout << "0 ";
    for (const auto& event : timeline) {
        totalTime += event.first;
        cout << totalTime << " ";
    }
    cout << "\n\n";
}

void displayResults(const vector<Process>& processes, const string& algorithm) {
    cout << "Results for " << algorithm << ":\n";
    cout << "-------------------------------------------\n";
    cout << "PID\tStart\tFinish\tWaiting\tTurnaround\n";
    cout << "-------------------------------------------\n";

    float totalWaitingTime = 0.0;
    float totalTurnaroundTime = 0.0;
    for (const auto& process : processes) {
        cout << process.pid << "\t"
            << process.startTime << "\t"
            << process.finishTime << "\t"
            << process.waitingTime << "\t"
            << process.turnaroundTime << endl;
        totalWaitingTime += process.waitingTime;
        totalTurnaroundTime += process.turnaroundTime;
    }

    cout << "-------------------------------------------\n";
    cout << "Average Waiting Time: " << totalWaitingTime / processes.size() << endl;
    cout << "Average Turnaround Time: " << totalTurnaroundTime / processes.size() << endl;
    cout << "-------------------------------------------\n";
    cout << endl;
}
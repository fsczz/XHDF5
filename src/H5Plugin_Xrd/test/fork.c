#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <mutex>

#define BUFFER_SIZE 1024

std::mutex mtx; // 保护共享资源的互斥锁

void thread_function(int thread_num) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid == 0) {
        // 子进程
        close(pipefd[0]); // 关闭读端

        // 子进程生成静态数据
        std::string data = "This is some static data for thread " + std::to_string(thread_num);
        const char* data_str = data.c_str();
        size_t size = data.size();

        // 将数据写入管道
        write(pipefd[1], &size, sizeof(size_t));
        write(pipefd[1], data_str, size);

        std::cout << "Child process " << thread_num << " wrote data: " << data_str << "  size: " << size << std::endl;
        close(pipefd[1]); // 写完后关闭写端
        exit(0);
    } else {
        // 父进程
        close(pipefd[1]); // 关闭写端

        // 从管道中读取数据
        size_t size;
        read(pipefd[0], &size, sizeof(size_t));
        char dataset_info_str[BUFFER_SIZE];
        read(pipefd[0], dataset_info_str, size);
        dataset_info_str[size] = '\0'; // 确保字符串终止

        {
            std::lock_guard<std::mutex> lock(mtx); // 确保打印输出的同步
            std::cout << "Parent process received: " << dataset_info_str << "  size: " << size << std::endl;
        }

        close(pipefd[0]); // 读完后关闭读端

        // 等待子进程完成
        wait(NULL);
    }
}

int main() {
    // 启动多个线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(thread_function, i + 1);
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}

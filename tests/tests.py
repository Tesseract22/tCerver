import requests

url = "http://127.0.0.1:8080/"
print(url)
mean_time = 0
N=100
for i in range(N):
    print(i)
    response = requests.post(url)
    mean_time += response.elapsed.total_seconds()
mean_time /= N
print(mean_time)
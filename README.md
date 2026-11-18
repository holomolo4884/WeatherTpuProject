# Курсовой проект по теме POCO - создать HTTP-клиент для выполнения GET-запросов к удалённому серверу и вывода полученных данных

## Команды для запуска и проверки сервера-заглушки

```bash
./build/weather_server
curl -i "http://localhost:8080/weather?city=Moscow"   # 200 и заглушка
curl -i "http://localhost:8080/weather"               # 400
curl -i "http://localhost:8080/abc"                   # 404
```

слушать порт lsof -i: 8080

TASK: System testów obcążeniowych dla serwerów sieciowych.

Celem projektu było zaimplementowanie narzędzia umożliwiającego testowanie obciążenia serwerów sieciowych.
Zadanie wykonano poprzez pomiar czasu odpowiedzi serwera na przesyłane żądania protokołu HTTP.
Użytkownik określa typ żądań (GET, POST), nazwę testowanego serwera, liczbę zapytań oraz liczbę wątków realizujących zapytania (zapytania są rozdzielane między wątki równomiernie).
Wynikiem działania programu jest wyznaczony minimalny, średni i maksymalny czas odpowiedzi serwera na przesłane zapytania, mediana czasów odpowiedzi oraz całkowity czas przeprowadzania pomiarów.

FILES: main.cpp -> zawiera cały kod programu

COMPILE: g++ main.c -o main

START: ./main host port request_number thread_number

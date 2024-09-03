# MonitoringServer
1초마다 Server들의 Monitoring 정보를 수집하여 외부 Monitoring Client들에게 전송

모니터링 정보 확인용으로 1분마다 DB에 Monitoring 정보를 저장
-> MySQL의 저장 프로시저를 사용하여 DB 부하 최소화

# BARN AI BSP

Zynq-7000 (Digilent Zybo Z7-20) 보드에서 FPGA 팀이 설계한 PL 회로를 리눅스가 인식하고 제어할 수 있게 만드는 PetaLinux BSP 작업입니다.

## 배경

FPGA PL에 새로 만들어 넣은 커스텀 IP는 자동 연결 기능이 없습니다. 정해진 물리 주소에 배치되어 있을 뿐이라, 커널이 버스에서 이를 찾아 연결할 방법이 없습니다. 그래서 그 PL 선언과 드라이버가 한 쌍으로 필요합니다.
이 프로젝트가 DTS와 드라이버라는 두 축으로 진행되는 이유입니다.

## 핵심 아이디어

주소는 전부 `.xsa`를 기반으로 합니다. Vivado가 추출한 `.xsa`를 PetaLinux가 읽어 디바이스 트리를 만들고, 드라이버는 거기서 주소를 받아옵니다. 코드에 주소를 직접 적을 일이 없습니다.

VDMA 같은 표준 IP는 커널에 드라이버가 이미 있습니다. 그래서 dmaengine 클라이언트를 사용했고, 커스텀 IP인 `axil_regfile`를 직접 개발했습니다.

## 진행 상황

### 완료

**`axi-gpio-drv`** — 4비트 AXI GPIO 입력을 sysfs `value`로 노출하는 첫 드라이버

- 목적: probe / `ioremap` / sysfs 기본 뼈대 익히기
- 문제: 커널 내장 `gpio-xilinx`가 기본 `compatible`을 먼저 선점
- 해결: `system-user.dtsi`에서 `compatible` 오버라이드

**VDMA 인터럽트 하드웨어 이슈** — 드라이버로는 손쓸 수 없던 배선 결함

- 증상: 구버전 `.xsa`에서 probe 자체가 실패 (`failed to get irq`)
- 원인: `s2mm_introut`이 PS의 `IRQ_F2P`에 미배선
- 대응: `.hwh` 증거를 첨부한 수정 요청서를 FPGA 팀에 전달
- 결과: 새 `.xsa`에서 반영된 것을 세 가지 증거로 재확인, 블로커 해소

**`vdma-capture`** — VDMA 레지스터를 직접 만지지 않는 dmaengine 클라이언트

- 검증: QEMU에서 probe 성공, sysfs 캡처 트리거까지 end-to-end 확인
- 발견한 버그: `wait_for_completion_timeout()` 리턴값 미체크로 타임아웃이 성공 로그로 찍히던 문제 → 수정 완료

**`axil_regfile` 레지스터 맵 확정** — 오프셋 정보가 어디에도 없던 IP

- 문제: `.hwh`와 `component.xml` 모두 오프셋 미기재
- 해결: FPGA 팀 저장소의 스펙 문서 · RTL 소스 · 검증용 소프트웨어 세 곳을 교차검증
- 부수 발견: v2부터 `CTRL` 리셋값이 0 → 소프트웨어가 명시적으로 켜기 전엔 TPG가 프레임을 만들지 않음

**`axil-regfile` 드라이버** — 스켈레톤에서 시작해 직접 작성

- 노출 속성: `id`(RO), `ctrl`(RW — TPG enable / mux select)
- 검증: QEMU probe 성공, sysfs 경로 예측 적중, `ctrl` read/write 정상

**SCD41 커널 설정**

- `CONFIG_SCD4X`, `CONFIG_CRC8` 활성화 후 `.cfg` 프래그먼트에 반영

* VIVA for visual Editor"

viva Editor는 C언어를 사용하여 작성된 간단한 텍스트 편집기 입니다. 기본적인 텍스트 편집 및 커서이동, 문자열 검색기능을 제공합니다.

#컴파일: 
윈도우, 맥os, linux에서 make를 입력하면 컴파일 되어 프로그램이 생성됩니다. 컴파일에 필요한 PdCurses는 zip에 함께 묶어서 배포하였습니다. make에 해당 경로를 지정해두었기 때문에 따로 수정이 필요하지 않습니다.

- mac os, linux의 경우 ncurses 패키지를 직접 설치해주세요.

#실행
window에서는 viva, mac os와 linux에서는 ./viva를 통해 실행 할 수 있습니다.

만약 기존 파일을 열고 싶으시다면, viva 파일이름.확장자명(window) 혹은 ./viva 파일이름 확장자명(mac, Linux)로 실행할 수 있습니다.
(* 단, 파일 명에 " "을 사용하지 마세요)

# 주요기능
1. 텍스트 편집 : 영어로만 입력이 가능합니다. 기본적인 텍스트 편집입니다.

2. 저장 및 종료:
 - 파일을 수정하고 저장하시려면 ctrl+s를 눌려주세요. 만약, 저장하지 않고 나가고 싶으시다면, ctrl+q를 연속해서 두번 눌러주시면 됩니다.
ctrl+q를 한번 누르는 경우 아래에서 네번째 줄에 경고 표시가 나옵니다.

3. 검색기능
- ctrl + f 키를 입력하면 검색모드로 들어갑니다. 검색할 단어는 맨 아래에서 세번 째 줄에서 입력을 받습니다.
검색이 완료되면, 찾은 문자열의 가장 첫번째 문자가 커서로 표기됩니다. 방향키 상하좌우로 다음 탐색결과로 커서를 이동할 수 있습니다.

- Enter를 누르면 현재 탐색이 종료되며 현재 커서에서부터 수정이 가능합니다.
- esc는 탐색을 포기하고 이전 커서로 이동합니다.

4. 커서이동
- 기본적으로 방향키로 이동이 가능합니다.
- end를 사용할 경우 현재 줄의 가장 우측으로 이동합니다.
- home을 사용할 경우 현재 줄의 가장 좌측으로 이동합니다.

### 참고사항
1. 인덱스는 row = 1, col = 1부터 시작되며, 노드를 입력하면 해당 노드가 가장 첫번째 열이어도 col = 2로 표기되는 점을 유의하세요.

2. 커서 이동이 불안정하여 프로그램이 의도치 않게 종료될 수 있습니다. 

3. 자료구조를 더블링크드 리스트를 사용하였기 때문에 많은 문자열을 입력하는 경우 메모리 오베헤드가 일어날 수 있습니다. 간단한
메모에만 사용해주세요.

4. 이 프로그램은 언제든지 무단 배폭가 가능합니다.
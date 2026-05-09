# 📋 Ruleset Guide — shield-str

커스텀 마스킹 룰을 JSON으로 작성하는 방법을 설명합니다.

---

## JSON 포맷

```json
{
  "version": "1.0",
  "rules": [
    {
      "id": "unique_rule_id",
      "description": "룰 설명 (선택)",
      "trigger_keywords": ["keyword1", "keyword2"],
      "pattern": "RE2 정규식 패턴",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
}
```

### 필드 설명

| 필드 | 필수 | 설명 |
|---|---|---|
| `id` | ✅ | 룰 고유 식별자 (문자열) |
| `description` | ❌ | 룰 설명 (문서화 목적) |
| `trigger_keywords` | ✅ | Aho-Corasick 탐색 키워드 목록. 이 키워드 중 하나라도 없으면 RE2 호출 생략 |
| `pattern` | ✅ | RE2 정규식. `\` 이스케이프는 JSON 문자열이므로 `\\` 로 작성 |
| `mask_group` | ❌ | 마스킹할 캡처 그룹 번호 (기본값: `0` = 전체 매치) |
| `replacement` | ❌ | 치환 문자열 (기본값: `"***"`) |

---

## mask_group 이해

```
입력:    "password=Secret123"
패턴:    "password=(\S+)"      ← group(0)=전체, group(1)=Secret123
```

| `mask_group` | 마스킹 범위 | 결과 |
|---|---|---|
| `0` | 전체 매치 | `***` |
| `1` | 그룹 1만 | `password=***` |

---

## 기본 내장 룰셋 5종

### rule_password

```json
{
  "id": "rule_password",
  "trigger_keywords": ["password", "passwd", "pwd"],
  "pattern": "(?i)(?:password|passwd|pwd)[\"']?\\s*[:=]\\s*[\"']?(\\S+)",
  "mask_group": 1,
  "replacement": "***"
}
```
**매칭 예:** `password=Secret` → `password=***`

---

### rule_bearer_token

```json
{
  "id": "rule_bearer_token",
  "trigger_keywords": ["Bearer", "Authorization"],
  "pattern": "Bearer\\s+([A-Za-z0-9\\-._~+/]+=*)",
  "mask_group": 1,
  "replacement": "***REDACTED***"
}
```
**매칭 예:** `Authorization: Bearer eyJ...` → `Authorization: Bearer ***REDACTED***`

---

### rule_email

```json
{
  "id": "rule_email",
  "trigger_keywords": ["@"],
  "pattern": "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}",
  "mask_group": 0,
  "replacement": "***@***.***"
}
```
**매칭 예:** `user@example.com` → `***@***.***`

---

### rule_phone_kr

```json
{
  "id": "rule_phone_kr",
  "trigger_keywords": ["010", "011", "016", "017", "018", "019"],
  "pattern": "01[016789]-?(\\d{3,4})-?(\\d{4})",
  "mask_group": 0,
  "replacement": "***-****-****"
}
```
**매칭 예:** `010-1234-5678` → `***-****-****`

---

### rule_credit_card

```json
{
  "id": "rule_credit_card",
  "trigger_keywords": ["card", "credit"],
  "pattern": "\\b(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})\\b",
  "mask_group": 0,
  "replacement": "****-****-****-****"
}
```
**매칭 예:** `4111-1111-1111-1111` → `****-****-****-****`

---

## 커스텀 룰 추가 예시

### API Key 마스킹

```json
{
  "id": "rule_api_key",
  "description": "X-API-Key 헤더 값 마스킹",
  "trigger_keywords": ["X-API-Key", "api_key", "apikey"],
  "pattern": "(?i)(?:X-API-Key|api[_-]?key)[\"']?\\s*[:=]\\s*[\"']?([A-Za-z0-9\\-_]{16,})",
  "mask_group": 1,
  "replacement": "***API_KEY_HIDDEN***"
}
```

---

## 트리거 키워드 설계 팁

> 트리거 키워드는 RE2 호출 여부를 결정하는 **Fast-Path 필터**입니다.

- **너무 넓은 키워드** (예: `"a"`) → 거의 모든 입력에서 RE2 호출 → 성능 저하
- **너무 좁은 키워드** → 일부 입력 누락 (False Negative) 위험
- **권장:** 실제 로그에서 민감 정보 앞에 반드시 등장하는 고유 문자열 사용

```json
// 좋은 예: 구체적인 필드명
"trigger_keywords": ["password", "passwd", "pwd"]

// 나쁜 예: 너무 짧아 오탐 발생
"trigger_keywords": ["p", "pw"]
```

---

## 정규식 작성 팁 (RE2)

- RE2는 `(?i)` 케이스 인센서티브 플래그 지원
- RE2는 Lookahead/Lookbehind **미지원** → 대신 캡처 그룹 + `mask_group` 활용
- `std::regex` Catastrophic Backtracking 없음 — 탐욕적 패턴 안전
- JSON 문자열에서 `\` → `\\` 로 이스케이프 필요

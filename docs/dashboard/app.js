// Simulated Rules for Demo matching C++ std::regex logic
const rules = [
    {
        pattern: /(?:password|passwd|pwd)["']?\s*[:=]\s*["']?([^\s"']+)/gi,
        maskGroup: 1,
        replacement: "***"
    },
    {
        pattern: /(?:authtoken|authcode|auth_token|auth_code)["']?\s*[:=]\s*["']?([^\s"']+)/gi,
        maskGroup: 1,
        replacement: "***AUTH***"
    },
    {
        pattern: /Bearer\s+([A-Za-z0-9\-._~+/]+={0,2})/g,
        maskGroup: 1,
        replacement: "***REDACTED***"
    },
    {
        pattern: /01[016789]-?(\d{3,4})-?(\d{4})/g,
        maskGroup: 0, // In JS simulation, we'll just replace the whole match if maskGroup is 0
        replacement: "***-****-****"
    }
];

document.addEventListener('DOMContentLoaded', () => {
    const logInput = document.getElementById('log-input');
    const logOutput = document.getElementById('log-output');

    const examples = {
        plain: `2026-05-09T10:00:00Z INFO User login attempt password=secret_pw123!\nAuthorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.abc\nToken update: authtoken=X1Y2Z3A4B9\nContact update: phone=010-1234-5678`,
        json: `{\n  "timestamp": "2026-05-09T10:00:00Z",\n  "level": "INFO",\n  "event": "login",\n  "data": {\n    "user": "alice",\n    "password": "superSecretPassword123",\n    "auth_token": "abc123def456",\n    "token": "Bearer abc123def456"\n  }\n}`,
        colon: `Status: FAILED\nReason: invalid credentials\npassword: eondlkajsdfasd\nusername: bob\nauthcode : secr3t_c0de\nphone: 010-9876-5432`
    };

    window.loadExample = function(type) {
        if (examples[type]) {
            logInput.value = examples[type];
            updateOutput();
        }
    };

    // Sample data to start
    logInput.value = examples.plain;

    function applyMasking(text) {
        let resultHtml = text;

        // Simple HTML escaping before we inject tags
        resultHtml = resultHtml.replace(/&/g, '&amp;')
                               .replace(/</g, '&lt;')
                               .replace(/>/g, '&gt;');

        // For simulation, we'll process rules sequentially.
        // In real C++ backend, it uses Aho-Corasick + RE2/std::regex.
        
        rules.forEach(rule => {
            // Because we escaped HTML, we need to be careful.
            // Assuming no HTML tags in our target strings for this simple demo.
            resultHtml = resultHtml.replace(rule.pattern, (match, ...args) => {
                if (rule.maskGroup === 0) {
                    return `<span class="masked-text">${rule.replacement}</span>`;
                } else {
                    // Extract the capturing groups from args
                    // args contains all groups, then offset, then full string
                    const groups = args.slice(0, args.length - 2);
                    
                    if (groups.length >= rule.maskGroup) {
                        const targetVal = groups[rule.maskGroup - 1];
                        if (targetVal) {
                            const offset = match.indexOf(targetVal);
                            const before = match.substring(0, offset);
                            const after = match.substring(offset + targetVal.length);
                            return `${before}<span class="masked-text">${rule.replacement}</span>${after}`;
                        }
                    }
                }
                return match;
            });
        });

        return resultHtml;
    }

    function updateOutput() {
        logOutput.innerHTML = applyMasking(logInput.value);
    }

    logInput.addEventListener('input', updateOutput);
    
    // Initial trigger
    updateOutput();
});

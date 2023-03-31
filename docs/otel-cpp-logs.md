# Stable OTEL logging APIs for C++

## Design Goals

1. Add minimum runtime cost if log is not turned on. **MUST**
2. Turn on/off portion of logs at runtime efficiently. **MUST**
3. Allow users to be highly efficient. **MUST**
4. Encourage structured logging. **MUST**

5. Context aware (like otel trace). **MUST**
6. Embrace message template. [1][1] **SHOULD**

[1]: https://messagetemplates.org/

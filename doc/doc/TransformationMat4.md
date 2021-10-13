Creates a 4x3 transformation matrix from the translation vector __p__, the 3x3 rotation Matrix __m__ (or YXZ euler rotation vector __e__) and the scaling vector __s__.

This is a more efficient version of `TranslationMat4(p) *  ScaleMat4(s) * m`
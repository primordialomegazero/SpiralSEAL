package main

import (
	"encoding/json"
	"net/http"
	"time"
)

// ═══════════════════════════════════════════
// ENTERPRISE HANDLERS
// ═══════════════════════════════════════════

func enterpriseHealthHandler(w http.ResponseWriter, r *http.Request) {
	if !enterprise.rateLimiter.Allow() {
		http.Error(w, `{"error":"rate limited","retry_after":"φ-decay"}`, 429)
		return
	}
	
	if !enterprise.circuitBrk.Allow() {
		http.Error(w, `{"error":"circuit breaker open","state":"`+enterprise.circuitBrk.state+`"}`, 503)
		return
	}
	
	enterprise.pool.Acquire()
	defer enterprise.pool.Release()
	
	atomic.AddUint64(&enterprise.ops, 1)
	enterprise.circuitBrk.RecordSuccess()
	
	stats := enterprise.Stats()
	json.NewEncoder(w).Encode(stats)
}

func enterpriseEncryptHandler(w http.ResponseWriter, r *http.Request) {
	var req struct {
		Plaintext string `json:"plaintext"`
		Purpose   string `json:"purpose"`
	}
	json.NewDecoder(r.Body).Decode(&req)
	
	// PQ key derivation
	key, salt := enterprise.pqKeyDeriv.DeriveKey(req.Plaintext, 32)
	
	// AES-256-GCM encryption with φ-IV
	ciphertext, err := encryption.Encrypt([]byte(req.Plaintext))
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
	
	// Multi-harmonic mixing
	mixed := enterprise.harmonics.Mix(ciphertext)
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"ciphertext":      hex.EncodeToString(mixed),
		"salt":            hex.EncodeToString(salt),
		"key_fingerprint": hex.EncodeToString(key[:8]),
		"harmonics":       []string{"φ", "π", "e", "√2"},
		"algorithm":       "AES-256-GCM-φ + Argon2id + Harmonic Mixing",
		"post_quantum":    true,
	})
}

func enterpriseDecryptHandler(w http.ResponseWriter, r *http.Request) {
	var req struct {
		Ciphertext string `json:"ciphertext"`
	}
	json.NewDecoder(r.Body).Decode(&req)
	
	data, err := hex.DecodeString(req.Ciphertext)
	if err != nil {
		http.Error(w, err.Error(), 400)
		return
	}
	
	plaintext, err := encryption.Decrypt(data)
	if err != nil {
		http.Error(w, err.Error(), 500)
		enterprise.circuitBrk.RecordFailure()
		atomic.AddUint64(&enterprise.errors, 1)
		return
	}
	
	enterprise.circuitBrk.RecordSuccess()
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"plaintext": string(plaintext),
		"decrypted": true,
	})
}

func enterpriseShardHandler(w http.ResponseWriter, r *http.Request) {
	key := r.URL.Query().Get("key")
	if key == "" { key = fmt.Sprintf("%d", time.Now().UnixNano()) }
	
	shard := enterprise.pool.GetShard(key)
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"key":         key,
		"shard":       shard,
		"total_shards": SHARD_COUNT,
		"phi_weight":   enterprise.pool.phiWeights[shard],
		"algorithm":    "φ-weighted consistent hashing",
	})
}

func enterpriseHarmonicsHandler(w http.ResponseWriter, r *http.Request) {
	var req struct {
		Data    string `json:"data"`
		Rounds  int    `json:"rounds"`
	}
	json.NewDecoder(r.Body).Decode(&req)
	if req.Rounds <= 0 { req.Rounds = 7 }
	
	engine := NewHarmonicsEngine()
	engine.rounds = req.Rounds
	mixed := engine.Mix([]byte(req.Data))
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"original":    req.Data,
		"mixed":       hex.EncodeToString(mixed),
		"rounds":      req.Rounds,
		"harmonics":   []string{"φ", "π", "e", "√2"},
		"lyapunov":    LYAPUNOV,
	})
}

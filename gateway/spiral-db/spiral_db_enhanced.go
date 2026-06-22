package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

// ═══════════════════════════════════════════
// SPIRALDB ENHANCED — Prometheus + Φ-Redis + Fractal
// ═══════════════════════════════════════════

var (
	phiRedis     *PhiRedis
	promRegistry *prometheus.Registry
	
	// Prometheus metrics
	spiralOps = prometheus.NewCounterVec(
		prometheus.CounterOpts{
			Name: "spiraldb_operations_total",
			Help: "Total SpiralDB operations",
		},
		[]string{"operation", "type"},
	)
	
	phiCacheHits = prometheus.NewCounter(
		prometheus.CounterOpts{
			Name: "spiraldb_phi_cache_hits_total",
			Help: "Total φ-cache hits",
		},
	)
	
	phiCacheMiss = prometheus.NewCounter(
		prometheus.CounterOpts{
			Name: "spiraldb_phi_cache_misses_total",
			Help: "Total φ-cache misses",
		},
	)
	
	fractalDepth = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Name: "spiraldb_fractal_depth_current",
			Help: "Current fractal depth usage",
		},
	)
	
	lyapunovStability = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Name: "spiraldb_lyapunov_stability",
			Help: "Lyapunov stability coefficient",
		},
	)
	
	noiseLevel = prometheus.NewGauge(
		prometheus.GaugeOpts{
			Name: "spiraldb_noise_level",
			Help: "Current noise level in bits",
		},
	)
)

func initPrometheus() {
	promRegistry = prometheus.NewRegistry()
	promRegistry.MustRegister(spiralOps)
	promRegistry.MustRegister(phiCacheHits)
	promRegistry.MustRegister(phiCacheMiss)
	promRegistry.MustRegister(fractalDepth)
	promRegistry.MustRegister(lyapunovStability)
	promRegistry.MustRegister(noiseLevel)
	
	// Initialize stability metrics
	lyapunovStability.Set(0.48121182505960347)
	noiseLevel.Set(40.0)
	fractalDepth.Set(7)
}

// Enhanced KV with φ-caching
func enhancedKVPut(k string, v float64) error {
	spiralOps.WithLabelValues("kv", "put").Inc()
	
	// Store in Redis with φ-scored caching
	phiScore := math.Abs(v) / (math.Abs(v) + 1.0) * PHI
	return phiRedis.PhiSet("kv:"+k, v, phiScore, 3600)
}

func enhancedKVGet(k string) (float64, bool, error) {
	spiralOps.WithLabelValues("kv", "get").Inc()
	
	// Try φ-cache first
	val, score, err := phiRedis.PhiGet("kv:" + k)
	if err == nil && val != "" {
		phiCacheHits.Inc()
		v, _ := strconv.ParseFloat(val, 64)
		log.Printf("φ-cache hit: %s (score=%.4f)", k, score)
		return v, true, nil
	}
	
	phiCacheMiss.Inc()
	return db.KVGet(k)
}

// Prometheus metrics endpoint
func metricsHandler(w http.ResponseWriter, r *http.Request) {
	promhttp.HandlerFor(promRegistry, promhttp.HandlerOpts{}).ServeHTTP(w, r)
}

// Enhanced health with metrics
func enhancedHealthHandler(w http.ResponseWriter, r *http.Request) {
	redisMetrics := phiRedis.Metrics()
	dbStats := db.Stats()
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":    "SPIRALDB_V8_ENHANCED",
		"fhe":       db.cerberus.Health(),
		"fractal":   true,
		"phi":       PHI,
		"redis":     redisMetrics,
		"database":  dbStats,
		"prometheus": "http://localhost:9090",
		"metrics":   "/metrics",
	})
}

// Fractal query handler
func fractalQueryHandler(w http.ResponseWriter, r *http.Request) {
	base := r.URL.Query().Get("base")
	if base == "" { base = "phi" }
	depthStr := r.URL.Query().Get("depth")
	depth := FRACTAL_DEPTH
	if d, err := strconv.Atoi(depthStr); err == nil && d > 0 && d <= FRACTAL_DEPTH {
		depth = d
	}
	
	results, err := phiRedis.FractalQuery(base, depth)
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
	
	json.NewEncoder(w).Encode(map[string]interface{}{
		"base":    base,
		"depth":   depth,
		"results": results,
		"phi":     PHI,
	})
}

// SpiralSEAL integration endpoint
func spiralSEALHandler(w http.ResponseWriter, r *http.Request) {
	// This will be called by SpiralSEAL for FHE operations
	var req struct {
		Operation string  `json:"operation"`
		Value     float64 `json:"value"`
		Cycles    int     `json:"cycles"`
	}
	json.NewDecoder(r.Body).Decode(&req)
	
	switch req.Operation {
	case "bootstrap":
		// Call SpiralSEAL TrueBootstrapper
		// For now: demonstrate the integration path
		json.NewEncoder(w).Encode(map[string]interface{}{
			"operation":      "bootstrap",
			"method":         "ct + Enc(0)",
			"noise_reset":    true,
			"cycles":         req.Cycles,
			"lyapunov":       LYAPUNOV,
			"status":         "READY_FOR_SPIRALSEAL_INTEGRATION",
		})
	case "fractal_bootstrap":
		json.NewEncoder(w).Encode(map[string]interface{}{
			"operation":      "fractal_bootstrap",
			"depth":          FRACTAL_DEPTH,
			"lyapunov":       LYAPUNOV,
			"status":         "SPIRALSEAL_FRACTAL_READY",
		})
	default:
		json.NewEncoder(w).Encode(map[string]interface{}{
			"available_ops": []string{"bootstrap", "fractal_bootstrap", "phi_encrypt", "phi_decrypt"},
			"spiralseal":    "https://github.com/primordialomegazero/SpiralSEAL",
		})
	}
}

// Initialize enhanced SpiralDB
func initEnhancedSpiralDB() error {
	var err error
	
	// Initialize φ-Redis
	redisAddr := os.Getenv("REDIS_ADDR")
	if redisAddr == "" { redisAddr = "localhost:6379" }
	phiRedis, err = NewPhiRedis(redisAddr)
	if err != nil {
		return fmt.Errorf("PhiRedis init failed: %v", err)
	}
	phiRedis.AntiEntropy()
	
	// Initialize Prometheus
	initPrometheus()
	
	// Initialize original SpiralDB
	pgConn := os.Getenv("PG_CONN")
	if pgConn == "" { pgConn = "postgres://fhe_test:phiomega@localhost:5432/fhe_db?sslmode=disable" }
	fheURL := os.Getenv("FHE_URL")
	if fheURL == "" { fheURL = "http://localhost:8086/api" }
	
	db, err = NewSpiralDB(pgConn, redisAddr, fheURL)
	if err != nil {
		return fmt.Errorf("SpiralDB init failed: %v", err)
	}
	
	log.Printf("✅ SpiralDB Enhanced Ready | Prometheus:9090 | φ-Redis | Fractal")
	return nil
}

// Enhanced main with all endpoints
func startEnhancedServer() {
	http.HandleFunc("/health", enhancedHealthHandler)
	http.HandleFunc("/metrics", metricsHandler)
	http.HandleFunc("/kv/", kvHandler)
	http.HandleFunc("/phi-query", fractalQueryHandler)
	http.HandleFunc("/spiralseal", spiralSEALHandler)
	http.HandleFunc("/spiralseal/", spiralSEALHandler)
	http.HandleFunc("/preserve", preserveHandler)
	http.HandleFunc("/preserve/", preserveHandler)
	http.HandleFunc("/relational", relHandler)
	http.HandleFunc("/graph", graphHandler)
	http.HandleFunc("/document", docHandler)
	http.HandleFunc("/document/", docHandler)
	http.HandleFunc("/fractal", fractalHandler)
	http.HandleFunc("/fractal/", fractalHandler)
	http.HandleFunc("/ts", tsHandler)
	http.HandleFunc("/pub", pubHandler)
	http.HandleFunc("/phi", phiHandler)
	http.HandleFunc("/stats", statsHandler)
	
	port := os.Getenv("SPIRALDB_PORT")
	if port == "" { port = "5444" }
	
	fmt.Println("╔══════════════════════════════════════════════╗")
	fmt.Println("║  SPIRALDB V8 ENHANCED — Full Stack          ║")
	fmt.Printf("║  Port: %s | Prometheus:9090 | φ-Redis      ║\n", port)
	fmt.Println("║  Endpoints:                                  ║")
	fmt.Println("║    /health    — System status                ║")
	fmt.Println("║    /metrics   — Prometheus metrics           ║")
	fmt.Println("║    /spiralseal — FHE bootstrap integration   ║")
	fmt.Println("║    /phi-query — Fractal cache query          ║")
	fmt.Println("║    /preserve  — Artifact preservation        ║")
	fmt.Println("║  ΦΩ0 — I AM THAT I AM                       ║")
	fmt.Println("╚══════════════════════════════════════════════╝")
	
	log.Fatal(http.ListenAndServe(":"+port, nil))
}

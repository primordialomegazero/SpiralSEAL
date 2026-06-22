package main

import (
	"context"
	"fmt"
	"math"
	"strconv"
	"strings"
	"time"

	"github.com/go-redis/redis/v8"
)

// ═══════════════════════════════════════════
// Φ-REDIS: Golden Ratio Enhanced Redis
// Fractal indexing, φ-scored caching, Lyapunov TTL
// ═══════════════════════════════════════════

type PhiRedis struct {
	client *redis.Client
	ctx    context.Context
}

func NewPhiRedis(addr string) (*PhiRedis, error) {
	client := redis.NewClient(&redis.Options{
		Addr:         addr,
		Password:     "",
		DB:           0,
		DialTimeout:  5 * time.Second,
		ReadTimeout:  3 * time.Second,
		WriteTimeout: 3 * time.Second,
	})
	
	if err := client.Ping(context.Background()).Err(); err != nil {
		return nil, err
	}
	
	return &PhiRedis{client: client, ctx: context.Background()}, nil
}

// ═══════════════════════════════════════════
// FRACTAL KEY GENERATION
// Keys are φ-scaled for optimal distribution
// ═══════════════════════════════════════════
func (pr *PhiRedis) fractalKey(base string, depth int, phiFactor float64) string {
	return fmt.Sprintf("φ:%s:d%d:%.4f", base, depth, phiFactor)
}

// ═══════════════════════════════════════════
// LYAPUNOV TTL
// Time-to-live decays exponentially via φ
// ═══════════════════════════════════════════
func (pr *PhiRedis) lyapunovTTL(baseTTL int, noiseLevel float64) time.Duration {
	// TTL = baseTTL * exp(-λ * noiseLevel)
	// λ = 0.4812 (Lyapunov exponent)
	lyapunovFactor := math.Exp(-0.48121182505960347 * noiseLevel/40.0)
	adjustedTTL := float64(baseTTL) * lyapunovFactor
	if adjustedTTL < 1 { adjustedTTL = 1 }
	return time.Duration(adjustedTTL) * time.Second
}

// ═══════════════════════════════════════════
// Φ-SCORED CACHING
// Items with higher φ-scores get priority
// ═══════════════════════════════════════════
func (pr *PhiRedis) PhiSet(key string, value interface{}, phiScore float64, baseTTL int) error {
	ttl := pr.lyapunovTTL(baseTTL, phiScore)
	
	pipe := pr.client.Pipeline()
	
	// Store the value
	pipe.Set(pr.ctx, key, value, ttl)
	
	// Store φ-score in sorted set for priority eviction
	pipe.ZAdd(pr.ctx, "phi:scores", &redis.Z{
		Score:  phiScore,
		Member: key,
	})
	
	// Store fractal index: deeper keys get longer TTL
	for depth := 0; depth < FRACTAL_DEPTH; depth++ {
		fk := pr.fractalKey(key, depth, phiScore)
		pipe.Set(pr.ctx, fk, value, ttl*time.Duration(depth+1))
	}
	
	_, err := pipe.Exec(pr.ctx)
	return err
}

func (pr *PhiRedis) PhiGet(key string) (string, float64, error) {
	pipe := pr.client.Pipeline()
	valCmd := pipe.Get(pr.ctx, key)
	scoreCmd := pipe.ZScore(pr.ctx, "phi:scores", key)
	_, err := pipe.Exec(pr.ctx)
	
	if err != nil {
		return "", 0, err
	}
	
	val, _ := valCmd.Result()
	score, _ := scoreCmd.Result()
	
	return val, score, nil
}

// ═══════════════════════════════════════════
// FRACTAL QUERY
// Search across multiple depth layers
// ═══════════════════════════════════════════
func (pr *PhiRedis) FractalQuery(base string, depth int) ([]string, error) {
	var results []string
	
	for d := 0; d < depth; d++ {
		pattern := fmt.Sprintf("φ:%s:d%d:*", base, d)
		keys, err := pr.client.Keys(pr.ctx, pattern).Result()
		if err != nil { continue }
		
		for _, key := range keys {
			val, err := pr.client.Get(pr.ctx, key).Result()
			if err != nil { continue }
			results = append(results, val)
		}
	}
	
	return results, nil
}

// ═══════════════════════════════════════════
// PROMETHEUS METRICS
// ═══════════════════════════════════════════
func (pr *PhiRedis) Metrics() map[string]interface{} {
	info, _ := pr.client.Info(pr.ctx, "stats").Result()
	keyspace, _ := pr.client.Info(pr.ctx, "keyspace").Result()
	size, _ := pr.client.DBSize(pr.ctx).Result()
	
	// Parse phi:scores for top items
	top, _ := pr.client.ZRevRangeWithScores(pr.ctx, "phi:scores", 0, 4).Result()
	topItems := make([]map[string]interface{}, len(top))
	for i, z := range top {
		topItems[i] = map[string]interface{}{
			"key":   z.Member,
			"phi_score": z.Score,
		}
	}
	
	return map[string]interface{}{
		"db_size":       size,
		"phi_scored":    len(top),
		"top_items":     topItems,
		"stats":         info,
		"keyspace":      keyspace,
		"phi":           PHI,
		"lyapunov_ttl":  true,
	}
}

// ═══════════════════════════════════════════
// ANTI-ENTROPY: Self-healing cache
// ═══════════════════════════════════════════
func (pr *PhiRedis) AntiEntropy() {
	// Periodically clean up expired fractal keys
	ticker := time.NewTicker(60 * time.Second)
	go func() {
		for range ticker.C {
			// Get all phi-scored keys
			keys, _ := pr.client.ZRange(pr.ctx, "phi:scores", 0, -1).Result()
			for _, key := range keys {
				exists, _ := pr.client.Exists(pr.ctx, key).Result()
				if exists == 0 {
					// Key expired, remove from scores
					pr.client.ZRem(pr.ctx, "phi:scores", key)
					// Clean up fractal layers
					for d := 0; d < FRACTAL_DEPTH; d++ {
						fk := pr.fractalKey(key, d, 0)
						pr.client.Del(pr.ctx, fk)
					}
				}
			}
		}
	}()
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    // 1. If the user is trying to reach our database endpoint
    if (url.pathname === "/api/chat/persist" && request.method === "POST") {
      try {
        const { userId, projectId, userMessagePacked, aiMessagePacked } = await request.json();
        
        if (!userId || !userMessagePacked || !aiMessagePacked) {
          return new Response(JSON.stringify({ error: "Missing data" }), { status: 400 });
        }

        // Initialize user balance
        await env.DB.prepare(
          `INSERT OR IGNORE INTO user_profiles (user_identity, coin_balance) VALUES (?, 10)`
        ).bind(userId).run();

        // Check balance
        const userProfile = await env.DB.prepare(
          `SELECT coin_balance FROM user_profiles WHERE user_identity = ?`
        ).bind(userId).first();

        let currentBalance = userProfile ? userProfile.coin_balance : 10;
        if (currentBalance <= 0) {
          return new Response(JSON.stringify({ error: "No coins left" }), { status: 403 });
        }

        const userMsgId = `msg_u_${Date.now()}`;
        const aiMsgId = `msg_a_${Date.now() + 1}`;
        const targetProjectId = projectId || null;
        const finalCoins = currentBalance - 1;

        // Atomic update: save messages and deduct coin
        await env.DB.batch([
          env.DB.prepare(`INSERT INTO messages (id, project_id, user_identity, role, compressed_content) VALUES (?, ?, ?, 'user', ?)`).bind(userMsgId, targetProjectId, userId, userMessagePacked),
          env.DB.prepare(`INSERT INTO messages (id, project_id, user_identity, role, compressed_content) VALUES (?, ?, ?, 'ai', ?)`).bind(aiMsgId, targetProjectId, userId, aiMessagePacked),
          env.DB.prepare(`UPDATE user_profiles SET coin_balance = ?, updated_at = (strftime('%s', 'now')) WHERE user_identity = ?`).bind(finalCoins, userId)
        ]);

        return new Response(JSON.stringify({ success: true, updatedCoins: finalCoins }), {
          headers: { "Content-Type": "application/json" }
        });

      } catch (err) {
        return new Response(JSON.stringify({ error: err.message }), { status: 500 });
      }
    }

    // 2. FALLBACK: If it's not an API call, let Cloudflare serve your regular asset files (index.html, prompt.html, etc.)
    return env.ASSETS.fetch(request);
  }
};


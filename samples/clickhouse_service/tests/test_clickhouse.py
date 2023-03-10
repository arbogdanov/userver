async def test_clickhouse(test_service_client):
    response = await test_service_client.request('GET', '/v1/db?limit=10')
    assert response.status_code == 200
    assert response.content == b'0123456789'

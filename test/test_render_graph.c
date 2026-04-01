#include "test_harness.h"
#include "physicalDevice.h"
#include "renderGraph.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static PhysicalDevice make_fake_pd(void)
{
    PhysicalDevice pd;
    memset(&pd, 0, sizeof(pd));
    return pd;
}

static RGResource make_ext_image(uintptr_t id, VkImageAspectFlags aspect)
{
    return (RGResource){
        .type       = RG_IMAGE,
        .format     = VK_FORMAT_B8G8R8A8_SRGB,
        .samples    = VK_SAMPLE_COUNT_1_BIT,
        .aspectMask = aspect,
        .isExternal = true,
        .image      = (VkImage)id,
        .imageView  = (VkImageView)id,
    };
}

// ---------------------------------------------------------------------------
// Topological sort tests
// ---------------------------------------------------------------------------

// Linear chain: Models → Lights → Particles
// Models writes gbuffer, Lights reads gbuffer + writes color, Particles reads color
void test_topo_linear_chain(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t gbufIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));
    uint32_t colIdx  = rgAddExternalResource(&rg, make_ext_image(2, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t models    = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t lights    = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t particles = rgAddPass(&rg, RG_PASS_GRAPHICS);

    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddOutput(&rg, lights, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgPassAddInput(&rg, particles, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    ASSERT_EQ(rg.executionOrderCount, 3);
    ASSERT_EQ(rg.executionOrder[0], models);
    ASSERT_EQ(rg.executionOrder[1], lights);
    ASSERT_EQ(rg.executionOrder[2], particles);
}

// Diamond: Shadow → Models and Shadow → Lights (both depend on shadow)
//   Shadow writes shadowmap
//   Models reads shadowmap, writes gbuffer
//   Lights reads shadowmap + gbuffer, writes color
// Expected: Shadow first, then Models, then Lights
void test_topo_diamond(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t shadowIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_DEPTH_BIT));
    uint32_t gbufIdx   = rgAddExternalResource(&rg, make_ext_image(2, VK_IMAGE_ASPECT_COLOR_BIT));
    uint32_t colIdx    = rgAddExternalResource(&rg, make_ext_image(3, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t shadow = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t models = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t lights = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Shadow writes shadowmap
    rgPassAddOutput(&rg, shadow, (RGResourceUsage){
        .resourceIndex = shadowIdx,
        .layout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .access        = RG_WRITE,
    });

    // Models reads shadowmap, writes gbuffer
    rgPassAddInput(&rg, models, (RGResourceUsage){
        .resourceIndex = shadowIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    // Lights reads shadowmap + gbuffer, writes color
    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = shadowIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddOutput(&rg, lights, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgCompileSortAndBarriers(&rg);

    ASSERT_EQ(rg.executionOrderCount, 3);
    ASSERT_EQ(rg.executionOrder[0], shadow);
    ASSERT_EQ(rg.executionOrder[1], models);
    ASSERT_EQ(rg.executionOrder[2], lights);
}

// No dependencies: 3 independent passes
// All should execute, order = registration order (all inDegree 0)
void test_topo_no_dependencies(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t r0 = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));
    uint32_t r1 = rgAddExternalResource(&rg, make_ext_image(2, VK_IMAGE_ASPECT_COLOR_BIT));
    uint32_t r2 = rgAddExternalResource(&rg, make_ext_image(3, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t p0 = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t p1 = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t p2 = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Each pass writes its own resource, no cross-dependencies
    rgPassAddOutput(&rg, p0, (RGResourceUsage){
        .resourceIndex = r0,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });
    rgPassAddOutput(&rg, p1, (RGResourceUsage){
        .resourceIndex = r1,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });
    rgPassAddOutput(&rg, p2, (RGResourceUsage){
        .resourceIndex = r2,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgCompileSortAndBarriers(&rg);

    ASSERT_EQ(rg.executionOrderCount, 3);
    // All have inDegree 0, so order matches registration order (BFS seed order)
    ASSERT_EQ(rg.executionOrder[0], p0);
    ASSERT_EQ(rg.executionOrder[1], p1);
    ASSERT_EQ(rg.executionOrder[2], p2);
}

// Reverse registration: register particles first, models last
// Sort should still produce correct dependency order
void test_topo_reverse_registration(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t gbufIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));

    // Register in reverse: particles first, models last
    uint32_t particles = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t models    = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Models writes gbuffer
    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    // Particles reads gbuffer
    rgPassAddInput(&rg, particles, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    ASSERT_EQ(rg.executionOrderCount, 2);
    // Models must come first despite being registered second
    ASSERT_EQ(rg.executionOrder[0], models);
    ASSERT_EQ(rg.executionOrder[1], particles);
}

// ---------------------------------------------------------------------------
// Barrier generation tests
// ---------------------------------------------------------------------------

// Linear chain: verify barriers on the reader pass
void test_barriers_linear_chain(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t gbufIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t models = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t lights = rgAddPass(&rg, RG_PASS_GRAPHICS);

    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    // Models pass: no barriers (first pass, output just updates tracking)
    ASSERT_EQ(rg.passes[models].imageBarrierCount, 0);

    // Lights pass: one barrier transitioning gbuffer
    ASSERT_EQ(rg.passes[lights].imageBarrierCount, 1);

    VkImageMemoryBarrier* b = &rg.passes[lights].imageBarriers[0];
    ASSERT_EQ(b->oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ASSERT_EQ(b->newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ASSERT_EQ(b->srcAccessMask, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    ASSERT_EQ(b->dstAccessMask, VK_ACCESS_SHADER_READ_BIT);

    ASSERT_EQ(rg.passes[lights].barrierSrcStageMask, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    ASSERT_EQ(rg.passes[lights].barrierDstStageMask, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

// First use: resource starts UNDEFINED, reader needs SHADER_READ_ONLY
// Should get barrier with srcAccess=0, srcStage=TOP_OF_PIPE
void test_barriers_first_use(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t texIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t pass = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Pass reads a resource that nobody wrote
    rgPassAddInput(&rg, pass, (RGResourceUsage){
        .resourceIndex = texIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    ASSERT_EQ(rg.passes[pass].imageBarrierCount, 1);

    VkImageMemoryBarrier* b = &rg.passes[pass].imageBarriers[0];
    ASSERT_EQ(b->oldLayout, VK_IMAGE_LAYOUT_UNDEFINED);
    ASSERT_EQ(b->newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ASSERT_EQ(b->srcAccessMask, 0);
    ASSERT_EQ(b->dstAccessMask, VK_ACCESS_SHADER_READ_BIT);

    ASSERT_EQ(rg.passes[pass].barrierSrcStageMask, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    ASSERT_EQ(rg.passes[pass].barrierDstStageMask, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

// No layout change needed: writer and reader use same layout
// Should produce no barrier
void test_barriers_same_layout_no_barrier(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t resIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t writer = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t reader = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Both use COLOR_ATTACHMENT_OPTIMAL
    rgPassAddOutput(&rg, writer, (RGResourceUsage){
        .resourceIndex = resIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });
    rgPassAddInput(&rg, reader, (RGResourceUsage){
        .resourceIndex = resIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    // Same layout → no image barrier
    ASSERT_EQ(rg.passes[reader].imageBarrierCount, 0);
}

// resolvedInitialLayout: first writer gets UNDEFINED, second writer gets previous layout
void test_barriers_resolved_initial_layout(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t colIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t pass0 = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t pass1 = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // pass0 writes color (first writer)
    rgPassAddOutput(&rg, pass0, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    // pass1 also writes color (reads it first to create dependency, then overwrites)
    rgPassAddInput(&rg, pass1, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddOutput(&rg, pass1, (RGResourceUsage){
        .resourceIndex = colIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    rgCompileSortAndBarriers(&rg);

    // pass0's output: resolvedInitialLayout should be UNDEFINED (first writer)
    ASSERT_EQ(rg.passes[pass0].outputs[0].resolvedInitialLayout, VK_IMAGE_LAYOUT_UNDEFINED);

    // pass1's output: resolvedInitialLayout should be SHADER_READ_ONLY_OPTIMAL
    // (the layout after pass1's own input barrier transitioned it)
    ASSERT_EQ(rg.passes[pass1].outputs[0].resolvedInitialLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

// Multiple resources: two barriers on the same reader pass
void test_barriers_multiple_resources(void)
{
    RenderGraph    rg;
    PhysicalDevice pd = make_fake_pd();
    rgInit(&rg, VK_NULL_HANDLE, &pd, 800, 600, 2);

    uint32_t gbufIdx = rgAddExternalResource(&rg, make_ext_image(1, VK_IMAGE_ASPECT_COLOR_BIT));
    uint32_t normIdx = rgAddExternalResource(&rg, make_ext_image(2, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t models = rgAddPass(&rg, RG_PASS_GRAPHICS);
    uint32_t lights = rgAddPass(&rg, RG_PASS_GRAPHICS);

    // Models writes both gbuffer and normals
    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });
    rgPassAddOutput(&rg, models, (RGResourceUsage){
        .resourceIndex = normIdx,
        .layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .accessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .stageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access        = RG_WRITE,
    });

    // Lights reads both
    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = gbufIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });
    rgPassAddInput(&rg, lights, (RGResourceUsage){
        .resourceIndex = normIdx,
        .layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .accessMask    = VK_ACCESS_SHADER_READ_BIT,
        .stageMask     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access        = RG_READ,
    });

    rgCompileSortAndBarriers(&rg);

    // Lights should have 2 image barriers (one per resource)
    ASSERT_EQ(rg.passes[lights].imageBarrierCount, 2);

    // Both should transition from COLOR_ATTACHMENT_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
    ASSERT_EQ(rg.passes[lights].imageBarriers[0].oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ASSERT_EQ(rg.passes[lights].imageBarriers[0].newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ASSERT_EQ(rg.passes[lights].imageBarriers[1].oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ASSERT_EQ(rg.passes[lights].imageBarriers[1].newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(void)
{
    printf("=== Render Graph Tests ===\n");

    printf("\n-- Topological Sort --\n");
    RUN_TEST(test_topo_linear_chain);
    RUN_TEST(test_topo_diamond);
    RUN_TEST(test_topo_no_dependencies);
    RUN_TEST(test_topo_reverse_registration);

    printf("\n-- Barrier Generation --\n");
    RUN_TEST(test_barriers_linear_chain);
    RUN_TEST(test_barriers_first_use);
    RUN_TEST(test_barriers_same_layout_no_barrier);
    RUN_TEST(test_barriers_resolved_initial_layout);
    RUN_TEST(test_barriers_multiple_resources);

    TEST_SUMMARY();
}
